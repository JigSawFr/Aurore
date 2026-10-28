/*
    Copyright 2026 JigSawFr

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <borealis/core/application.hpp>
#include <borealis/core/hot_reload.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/core/theme_image.hpp>
#include <borealis/core/thread.hpp>
#ifdef USE_LIBROMFS
#include <romfs/romfs.hpp>
#endif

#include <sys/stat.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <map>
#include <mutex>
#include <set>
#include <tuple>
#include <vector>

namespace brls
{

namespace
{

    // Decoded pixels, RGBA, made smaller if needed
    struct Pixels
    {
        std::vector<unsigned char> rgba;
        int width = 0, height = 0;
        NVGcolor mean {};
    };

    // Where a picture's bytes are: a file on disk, or bytes in memory (romfs)
    struct Source
    {
        std::string file;
        const unsigned char* data = nullptr;
        size_t size               = 0;
    };

    std::atomic<int> decodingCount { 0 };
    std::mutex meanMutex;
    std::map<std::string, NVGcolor> means; // by keyOf(), under meanMutex

    std::set<std::string> reported; // paths already logged as unreadable (UI thread)
    // By path, flags, size cap and mask (UI thread)
    using ImageKey = std::tuple<std::string, int, int, bool>;
    std::map<ImageKey, std::weak_ptr<ThemeImage>> images;
    Event<const ThemeImage*> loadedEvent;

    // sRGB to linear light, for an average the eye agrees with
    const float* linearTable()
    {
        // Made once, from whichever thread asks first (a static's
        // initialisation is thread-safe)
        static const std::vector<float> table = []
        {
            std::vector<float> values(256);
            for (int i = 0; i < 256; i++)
            {
                float v   = i / 255.0f;
                values[i] = v <= 0.04045f ? v / 12.92f : std::pow((v + 0.055f) / 1.055f, 2.4f);
            }
            return values;
        }();
        return table.data();
    }

    float toSRGB(float v)
    {
        v = std::clamp(v, 0.0f, 1.0f);
        return v <= 0.0031308f ? v * 12.92f : 1.055f * std::pow(v, 1.0f / 2.4f) - 0.055f;
    }

    // The average colour, each pixel weighed by its alpha, every few pixels
    // (a mean does not need them all)
    NVGcolor averageOf(const unsigned char* rgba, int width, int height)
    {
        const float* linear = linearTable();
        int step            = std::max(1, std::min(width, height) / 256);
        double r = 0, g = 0, b = 0, weight = 0;
        for (int y = 0; y < height; y += step)
        {
            const unsigned char* row = rgba + (size_t)y * width * 4;
            for (int x = 0; x < width; x += step)
            {
                const unsigned char* p = row + (size_t)x * 4;
                double a               = p[3] / 255.0;
                r += linear[p[0]] * a;
                g += linear[p[1]] * a;
                b += linear[p[2]] * a;
                weight += a;
            }
        }
        if (weight <= 0)
            return nvgRGB(0, 0, 0);
        return nvgRGBf(toSRGB((float)(r / weight)), toSRGB((float)(g / weight)), toSRGB((float)(b / weight)));
    }

    // An area average down to at most maxWidth x maxHeight, the ratio kept:
    // rows first, then columns
    void downscale(Pixels* pixels, const unsigned char* source, int width, int height, int maxWidth, int maxHeight)
    {
        float scale    = std::min(1.0f, std::min((float)maxWidth / width, (float)maxHeight / height));
        int outW       = std::max(1, (int)std::lround(width * scale));
        int outH       = std::max(1, (int)std::lround(height * scale));
        pixels->width  = outW;
        pixels->height = outH;
        if (outW == width && outH == height)
        {
            pixels->rgba.assign(source, source + (size_t)width * height * 4);
            return;
        }

        // The source spans [begin, end) of each output column (or row)
        auto spans = [](int in, int out)
        {
            std::vector<std::pair<int, int>> result(out);
            for (int i = 0; i < out; i++)
            {
                int begin = (int)((int64_t)i * in / out);
                int end   = std::max(begin + 1, (int)((int64_t)(i + 1) * in / out));
                result[i] = { begin, std::min(end, in) };
            }
            return result;
        };
        auto columns = spans(width, outW);
        auto rows    = spans(height, outH);

        // Premultiplied while averaging, so a transparent pixel's colour does
        // not bleed into its neighbours; one output row at a time, in 64 bits
        std::vector<uint64_t> sums((size_t)outW * 4);
        pixels->rgba.resize((size_t)outW * outH * 4);
        for (int y = 0; y < outH; y++)
        {
            std::fill(sums.begin(), sums.end(), 0);
            for (int sy = rows[y].first; sy < rows[y].second; sy++)
            {
                const unsigned char* row = source + (size_t)sy * width * 4;
                for (int x = 0; x < outW; x++)
                {
                    uint64_t* sum = sums.data() + (size_t)x * 4;
                    for (int sx = columns[x].first; sx < columns[x].second; sx++)
                    {
                        const unsigned char* p = row + (size_t)sx * 4;
                        sum[0] += p[0] * p[3];
                        sum[1] += p[1] * p[3];
                        sum[2] += p[2] * p[3];
                        sum[3] += p[3];
                    }
                }
            }

            unsigned char* out = pixels->rgba.data() + (size_t)y * outW * 4;
            uint64_t rowCount  = rows[y].second - rows[y].first;
            for (int x = 0; x < outW; x++)
            {
                const uint64_t* sum = sums.data() + (size_t)x * 4;
                uint64_t count      = rowCount * (uint64_t)(columns[x].second - columns[x].first);
                for (int c = 0; c < 3; c++)
                    out[x * 4 + c] = sum[3] ? (unsigned char)std::min<uint64_t>(255, sum[c] / sum[3]) : 0;
                out[x * 4 + 3] = (unsigned char)(sum[3] / count);
            }
        }
    }

    // A mask to tint: white, its alpha the picture's; a picture with no
    // transparent pixel keeps its dark parts (the ink of a drawing on paper)
    void makeMask(Pixels* pixels)
    {
        std::vector<unsigned char>& rgba = pixels->rgba;
        bool transparent                 = false;
        for (size_t i = 3; i < rgba.size() && !transparent; i += 4)
            transparent = rgba[i] < 255;
        for (size_t i = 0; i < rgba.size(); i += 4)
        {
            if (!transparent)
                rgba[i + 3] = (unsigned char)(255 - (rgba[i] * 54 + rgba[i + 1] * 183 + rgba[i + 2] * 19) / 256);
            rgba[i] = rgba[i + 1] = rgba[i + 2] = 255;
        }
    }

    // Decodes (any thread): false when it cannot
    bool decode(const Source& source, Pixels* pixels, int maxWidth, int maxHeight)
    {
        // Refused from its header when too large (NVG_MAX_IMAGE_PIXELS)
        int width = 0, height = 0;
        unsigned char* data = source.data ? nvgDecodeImageMem(source.data, (int)source.size, &width, &height)
                                          : nvgDecodeImage(source.file.c_str(), &width, &height);
        if (!data)
            return false;
        if (width <= 0 || height <= 0)
        {
            nvgFreeImageData(data);
            return false;
        }
        if (maxWidth > 0)
            downscale(pixels, data, width, height, maxWidth, maxHeight);
        else
        {
            pixels->width  = width;
            pixels->height = height;
        }
        pixels->mean = maxWidth > 0 ? averageOf(pixels->rgba.data(), pixels->width, pixels->height) : averageOf(data, width, height);
        nvgFreeImageData(data);
        return true;
    }

    // Where `path` is read from (UI thread for a resource: romfs is not
    // thread-safe); false when it is nowhere
    bool locate(const std::string& path, Source* source)
    {
        std::string disk        = HotReload::diskResource(path);
        const std::string& name = disk.empty() ? path : disk;
#ifdef USE_LIBROMFS
        if (name.rfind("@res/", 0) == 0)
        {
            try
            {
                const romfs::Resource& resource = romfs::get(name.substr(5));
                source->data                    = reinterpret_cast<const unsigned char*>(resource.data());
                source->size                    = resource.size();
                return source->size > 0;
            }
            catch (const std::exception& e)
            {
                return false;
            }
        }
#endif
        source->file = name.rfind("@res/", 0) == 0 ? std::string(BRLS_RESOURCES) + name.substr(5) : name;
        return true;
    }

    // The mean cache's key: a file changed on disk is another picture
    std::string keyOf(const std::string& path, const Source& source)
    {
        if (source.file.empty())
            return path;
        struct stat info;
        if (stat(source.file.c_str(), &info) != 0)
            return path;
        return path + "|" + std::to_string((long long)info.st_size) + "|" + std::to_string((long long)info.st_mtime);
    }

    void rememberMean(const std::string& key, NVGcolor mean)
    {
        std::lock_guard<std::mutex> lock(meanMutex);
        means[key] = mean;
    }

    bool recalledMean(const std::string& key, NVGcolor* mean)
    {
        std::lock_guard<std::mutex> lock(meanMutex);
        auto it = means.find(key);
        if (it == means.end())
            return false;
        *mean = it->second;
        return true;
    }

    void reportOnce(const std::string& path)
    {
        if (reported.insert(path).second)
            Logger::error("theme: cannot load the image {}", path);
    }

} // namespace

ThemeImage::~ThemeImage()
{
    if (this->texture)
        nvgDeleteImage(Application::getNVGContext(), this->texture);
}

void ThemeImages::releaseAll()
{
    for (auto& [key, weak] : images)
        if (std::shared_ptr<ThemeImage> image = weak.lock())
        {
            if (image->texture)
                nvgDeleteImage(Application::getNVGContext(), image->texture);
            image->texture = 0;
            image->state   = ThemeImage::State::FAILED;
        }
    images.clear();
}

Event<const ThemeImage*>* ThemeImages::getLoadedEvent()
{
    return &loadedEvent;
}

std::shared_ptr<ThemeImage> ThemeImages::load(const std::string& path, int flags)
{
    ThemeImageOptions options;
    options.flags = flags;
    return ThemeImages::load(path, options);
}

std::shared_ptr<ThemeImage> ThemeImages::load(const std::string& path, const ThemeImageOptions& options)
{
    int flags   = options.flags;
    int maxSide = std::max(0, options.maxSide);
    bool mask   = options.mask;
    ImageKey key { path, flags, maxSide, mask };
    auto it = images.find(key);
    if (it != images.end())
    {
        if (std::shared_ptr<ThemeImage> image = it->second.lock())
            return image;
        images.erase(it);
    }

    auto image     = std::make_shared<ThemeImage>();
    image->path    = path;
    image->flags   = flags;
    image->maxSide = maxSide;
    image->mask    = mask;
    images[key]    = image;

    Source source;
    if (!locate(path, &source))
    {
        image->state = ThemeImage::State::FAILED;
        reportOnce(path);
        return image;
    }

    std::string meanKey            = keyOf(path, source);
    std::weak_ptr<ThemeImage> weak = image;
    decodingCount++;
    int maxWidth  = maxSide > 0 ? std::min(maxSide, MAX_WIDTH) : MAX_WIDTH;
    int maxHeight = maxSide > 0 ? std::min(maxSide, MAX_HEIGHT) : MAX_HEIGHT;
    brls::async([weak, source, meanKey, maxWidth, maxHeight, mask]()
        {
            // Shared with the UI thread's task, freed by whichever ends last
            auto pixels = std::make_shared<Pixels>();
            bool ok     = decode(source, pixels.get(), maxWidth, maxHeight);
            // The mean of the whole picture only (the background's)
            if (ok && maxWidth == MAX_WIDTH && maxHeight == MAX_HEIGHT)
                rememberMean(meanKey, pixels->mean);
            if (ok && mask)
                makeMask(pixels.get());
            decodingCount--;
            brls::sync([weak, pixels, ok]()
                {
                    std::shared_ptr<ThemeImage> image = weak.lock();
                    if (!image)
                        return; // the theme changed meanwhile
                    int texture = 0;
                    if (ok)
                        texture = nvgCreateImageRGBA(Application::getNVGContext(), pixels->width, pixels->height,
                            image->flags, pixels->rgba.data());
                    if (texture)
                    {
                        image->texture = texture;
                        image->width   = pixels->width;
                        image->height  = pixels->height;
                        image->mean    = pixels->mean;
                        image->state   = ThemeImage::State::READY;
                    }
                    else
                    {
                        image->state = ThemeImage::State::FAILED;
                        reportOnce(image->path);
                    }
                    loadedEvent.fire(image.get());
                    Application::requestRedraw();
                }); });
    return image;
}

bool ThemeImage::draw(NVGcontext* vg, float x, float y, float width, float height, BackgroundFit fit,
    const NVGcolor* tint, float alpha, float corner) const
{
    if (!this->ready() || this->width <= 0 || this->height <= 0 || width <= 0 || height <= 0)
        return false;
    float w = (float)this->width, h = (float)this->height;
    float left = x, top = y, right = x + width, bottom = y + height; // what is filled
    float px = x, py = y; // where the picture is
    switch (fit)
    {
        case BackgroundFit::COVER:
        case BackgroundFit::CONTAIN:
        {
            float scale = fit == BackgroundFit::COVER ? std::max(width / w, height / h) : std::min(width / w, height / h);
            w *= scale;
            h *= scale;
            px = x + (width - w) / 2;
            py = y + (height - h) / 2;
            if (fit == BackgroundFit::CONTAIN)
            {
                left = px, top = py, right = px + w, bottom = py + h;
                corner = 0;
            }
            break;
        }
        case BackgroundFit::CENTER:
            px     = std::round(x + (width - w) / 2);
            py     = std::round(y + (height - h) / 2);
            left   = std::max(left, px);
            top    = std::max(top, py);
            right  = std::min(right, px + w);
            bottom = std::min(bottom, py + h);
            break;
        case BackgroundFit::TILE:
            break;
    }
    if (right <= left || bottom <= top)
        return true;
    NVGpaint paint = nvgImagePattern(vg, px, py, w, h, 0, this->texture, alpha);
    if (tint)
    {
        paint.innerColor = paint.outerColor = nvgRGBAf(tint->r, tint->g, tint->b, tint->a * alpha);
    }
    nvgBeginPath(vg);
    if (corner > 0)
        nvgRoundedRect(vg, left, top, right - left, bottom - top, corner);
    else
        nvgRect(vg, left, top, right - left, bottom - top);
    nvgFillPaint(vg, paint);
    nvgFill(vg);
    return true;
}

bool ThemeImages::exists(const std::string& path)
{
    Source source;
    if (!locate(path, &source))
        return false;
    struct stat info;
    return source.file.empty() || (stat(source.file.c_str(), &info) == 0 && (info.st_mode & S_IFMT) == S_IFREG);
}

int ThemeImages::decoding()
{
    return decodingCount;
}

bool ThemeImages::knownMeanColor(const std::string& path, NVGcolor* color)
{
    Source source;
    return locate(path, &source) && recalledMean(keyOf(path, source), color);
}

bool ThemeImages::meanColor(const std::string& path, NVGcolor* color)
{
    Source source;
    if (!locate(path, &source))
        return false;
    std::string key = keyOf(path, source);
    if (recalledMean(key, color))
        return true;
    Pixels pixels;
    if (!decode(source, &pixels, 0, 0))
        return false;
    rememberMean(key, pixels.mean);
    *color = pixels.mean;
    return true;
}

} // namespace brls

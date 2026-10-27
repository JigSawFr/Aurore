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

// Pictures larger than NVG_MAX_IMAGE_PIXELS are refused from their header,
// on every path that decodes one, before anything large is allocated

#include <borealis.hpp>
#include <borealis/core/theme_image.hpp>
#include <borealis/test/harness.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#ifdef __unix__
#include <sys/resource.h>
#endif

using brls::test::Harness;

namespace
{

// A PNG of width x height transparent pixels, its zlib stream made of
// 258-byte runs of zeros: a few megabytes that decode to width x height x 4
std::vector<unsigned char> zeroPng(uint32_t width, uint32_t height)
{
    struct Bits
    {
        std::vector<unsigned char> out;
        uint32_t acc = 0;
        int count    = 0;
        void put(uint32_t value, int bits)
        {
            acc |= value << count;
            count += bits;
            while (count >= 8)
            {
                out.push_back(acc & 0xff);
                acc >>= 8;
                count -= 8;
            }
        }
        // A Huffman code goes most significant bit first
        void code(uint32_t value, int bits)
        {
            uint32_t reversed = 0;
            for (int i = 0; i < bits; i++)
                reversed |= ((value >> i) & 1) << (bits - 1 - i);
            put(reversed, bits);
        }
    };

    uint64_t raw = (uint64_t)height * (1 + (uint64_t)width * 4); // a filter byte a row
    Bits deflate;
    deflate.out = { 0x78, 0x01 };
    deflate.put(1, 1); // the last block
    deflate.put(1, 2); // fixed codes
    deflate.code(0x30, 8); // literal 0
    uint64_t left = raw - 1;
    for (; left >= 258; left -= 258)
    {
        deflate.code(0xc5, 8); // length 258
        deflate.code(0, 5); // distance 1
    }
    for (; left > 0; left--)
        deflate.code(0x30, 8);
    deflate.code(0, 7); // end of block
    if (deflate.count)
        deflate.put(0, 8 - deflate.count);
    uint32_t adler = (uint32_t)(raw % 65521) << 16 | 1;
    for (int i = 3; i >= 0; i--)
        deflate.out.push_back((adler >> (i * 8)) & 0xff);

    auto crc32 = [](const unsigned char* data, size_t size)
    {
        uint32_t crc = 0xffffffff;
        for (size_t i = 0; i < size; i++)
        {
            crc ^= data[i];
            for (int k = 0; k < 8; k++)
                crc = (crc >> 1) ^ (0xedb88320 & (0 - (crc & 1)));
        }
        return ~crc;
    };
    std::vector<unsigned char> png = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n' };
    auto be32                      = [&](uint32_t v)
    {
        for (int i = 3; i >= 0; i--)
            png.push_back((v >> (i * 8)) & 0xff);
    };
    auto chunk = [&](const char* type, const std::vector<unsigned char>& data)
    {
        be32((uint32_t)data.size());
        size_t start = png.size();
        png.insert(png.end(), type, type + 4);
        png.insert(png.end(), data.begin(), data.end());
        be32(crc32(png.data() + start, png.size() - start));
    };
    std::vector<unsigned char> header;
    for (uint32_t v : { width, height })
        for (int i = 3; i >= 0; i--)
            header.push_back((v >> (i * 8)) & 0xff);
    header.insert(header.end(), { 8, 6, 0, 0, 0 }); // 8 bits, RGBA
    chunk("IHDR", header);
    chunk("IDAT", deflate.out);
    chunk("IEND", {});
    return png;
}

// The largest the process has been, in megabytes (0 where unknown)
long peakMegabytes()
{
#ifdef __unix__
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
#ifdef __APPLE__
    return usage.ru_maxrss / (1024 * 1024);
#else
    return usage.ru_maxrss / 1024;
#endif
#else
    return 0;
#endif
}

double millisecondsSince(std::chrono::steady_clock::time_point start)
{
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

struct TempDir
{
    std::filesystem::path dir;
    explicit TempDir(const char* name)
        : dir(std::filesystem::temp_directory_path() / name)
    {
        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir);
    }
    ~TempDir() { std::filesystem::remove_all(dir); }

    std::string write(const std::string& name, const std::vector<unsigned char>& bytes)
    {
        std::ofstream(dir / name, std::ios::binary).write((const char*)bytes.data(), (std::streamsize)bytes.size());
        return (dir / name).generic_string();
    }

    // A picture of one colour, opaque
    std::string picture(const std::string& name, unsigned char r, unsigned char g, unsigned char b, int width, int height)
    {
        brls::test::Image image;
        image.width  = width;
        image.height = height;
        image.rgba.reserve((size_t)width * height * 4);
        for (size_t i = 0; i < (size_t)width * height; i++)
            image.rgba.insert(image.rgba.end(), { r, g, b, 255 });
        image.savePng((dir / name).string());
        return (dir / name).generic_string();
    }
};

void settlePictures(Harness& h)
{
    for (int round = 0; round < 3; round++)
    {
        h.step();
        while (brls::ThemeImages::decoding() > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        h.settle();
    }
}

} // namespace

BRLS_TEST(bomb_is_refused_from_its_header)
{
    // 8192 x 8192: 256 MB of pixels, from 1.7 MB
    std::vector<unsigned char> bomb = zeroPng(8192, 8192);
    TempDir temp("aurore_image_limits_bomb");
    std::string path = temp.write("bomb.png", bomb);
    long before      = peakMegabytes();
    auto start       = std::chrono::steady_clock::now();

    int w = 0, h = 0;
    BRLS_CHECK(nvgDecodeImageMem(bomb.data(), (int)bomb.size(), &w, &h) == nullptr);
    BRLS_CHECK_EQ(w, 8192); // read from the header
    BRLS_CHECK(nvgDecodeImage(path.c_str(), &w, &h) == nullptr);
    NVGcolor mean;
    BRLS_CHECK(!brls::ThemeImages::meanColor(path, &mean));

    double elapsed = millisecondsSince(start);
    long grown     = peakMegabytes() - before;
    BRLS_CHECK(elapsed < 200);
    BRLS_CHECK(grown < 50);
    brls::Logger::info("bomb refused in {:.1f} ms, peak memory +{} MB", elapsed, grown);
}

BRLS_TEST(bomb_is_refused_by_the_views_and_theme_pictures)
{
    Harness harness;
    std::vector<unsigned char> bomb = zeroPng(8192, 8192);
    TempDir temp("aurore_image_limits_views");
    std::string path = temp.write("bomb.png", bomb);
    long before      = peakMegabytes();

    auto* image = new brls::Image();
    harness.push(new brls::Activity(image));
    image->setImageFromMem(bomb.data(), (int)bomb.size());
    BRLS_CHECK_EQ(image->getTexture(), 0);
    image->setImageFromFile(path);
    BRLS_CHECK_EQ(image->getTexture(), 0);
    image->setImageFromFileAsync(path);
    harness.settle();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    harness.settle();
    BRLS_CHECK_EQ(image->getTexture(), 0);

    std::shared_ptr<brls::ThemeImage> picture = brls::ThemeImages::load(path);
    settlePictures(harness);
    BRLS_CHECK(picture->state == brls::ThemeImage::State::FAILED);
    BRLS_CHECK(peakMegabytes() - before < 50);
}

BRLS_TEST(pictures_up_to_the_limit_still_load)
{
    Harness harness;
    TempDir temp("aurore_image_limits_ok");
    std::string jpeg = std::string(AURORE_SOURCE_RESOURCES) + "img/demo_icon.jpg";
    int w = 0, h = 0;
    unsigned char* pixels = nvgDecodeImage(jpeg.c_str(), &w, &h);
    BRLS_CHECK(pixels != nullptr && w > 0 && h > 0);
    nvgFreeImageData(pixels);

    // A transparent picture of exactly NVG_MAX_IMAGE_PIXELS
    std::vector<unsigned char> largest = zeroPng(NVG_MAX_IMAGE_SIDE, NVG_MAX_IMAGE_PIXELS / NVG_MAX_IMAGE_SIDE);
    pixels                             = nvgDecodeImageMem(largest.data(), (int)largest.size(), &w, &h);
    BRLS_CHECK(pixels != nullptr);
    BRLS_CHECK_EQ(w * h, NVG_MAX_IMAGE_PIXELS);
    nvgFreeImageData(pixels);
    std::vector<unsigned char> wider = zeroPng(NVG_MAX_IMAGE_SIDE + 1, 1);
    BRLS_CHECK(nvgDecodeImageMem(wider.data(), (int)wider.size(), &w, &h) == nullptr);

    auto* image = new brls::Image();
    harness.push(new brls::Activity(image));
    image->setImageFromFile(jpeg);
    BRLS_CHECK(image->getTexture() != 0);

    std::string png = temp.picture("red.png", 220, 20, 20, 300, 200);
    NVGcolor mean;
    BRLS_CHECK(brls::ThemeImages::meanColor(png, &mean));
    BRLS_CHECK(mean.r > 0.8f && mean.g < 0.15f);
}

BRLS_TEST(downscale_keeps_the_colour_over_wide_spans)
{
    // A wide picture down to a few pixels: each output pixel averages
    // thousands of source pixels (sums kept in 64 bits)
    Harness harness;
    TempDir temp("aurore_image_limits_downscale");
    std::string path = temp.picture("wide.png", 255, 255, 255, NVG_MAX_IMAGE_SIDE, 8);
    brls::ThemeImageOptions options;
    options.maxSide                           = 4;
    std::shared_ptr<brls::ThemeImage> picture = brls::ThemeImages::load(path, options);
    settlePictures(harness);
    BRLS_CHECK(picture->ready());
    BRLS_CHECK_EQ(picture->width, 4);
    BRLS_CHECK_EQ(picture->height, 1);
    BRLS_CHECK(picture->mean.r > 0.99f && picture->mean.g > 0.99f && picture->mean.b > 0.99f);
}

int main(int argc, char** argv)
{
    return brls::test::runTests(argc, argv);
}

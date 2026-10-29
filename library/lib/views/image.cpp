// SPDX-FileCopyrightText: 2019 WerWolv
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-FileCopyrightText: 2020-2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#include <yoga/Yoga.h>

#include <algorithm>
#include <climits>
#include <cmath>

#include <aurore/core/application.hpp>
#include <aurore/core/theme_image.hpp>
#include <aurore/core/util.hpp>
#include <aurore/views/image.hpp>

#include "aurore/core/cache_helper.hpp"
#include "aurore/core/thread.hpp"

#include <memory>

namespace aurore
{

static float measureWidth(YGNodeConstRef node, float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode, float originalWidth, ImageScalingType type)
{
    if (widthMode == YGMeasureModeUndefined)
        return originalWidth;
    else if (widthMode == YGMeasureModeAtMost)
        if (type == ImageScalingType::FIT)
            return originalWidth;
        else
            return std::min(width, originalWidth);
    else if (widthMode == YGMeasureModeExactly)
        return width;
    else
        fatal("Unsupported Image width measure mode: " + std::to_string(widthMode));

    return width;
}

static float measureHeight(YGNodeConstRef node, float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode, float originalHeight, ImageScalingType type)
{
    if (heightMode == YGMeasureModeUndefined)
        return originalHeight;
    else if (heightMode == YGMeasureModeAtMost)
        if (type == ImageScalingType::FIT)
            return originalHeight;
        else
            return std::min(height, originalHeight);
    else if (heightMode == YGMeasureModeExactly)
        return height;
    else
        fatal("Unsupported Image height measure mode: " + std::to_string(heightMode));

    return height;
}

static YGSize imageMeasureFunc(YGNodeConstRef node, float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode)
{
    Image* image                 = (Image*)YGNodeGetContext(node);
    int texture                  = image->getTexture();
    float originalWidth          = image->getOriginalImageWidth();
    float originalHeight         = image->getOriginalImageHeight();
    ImageScalingType scalingType = image->getScalingType();

    YGSize size = {
        .width  = width,
        .height = height,
    };

    if (texture == 0)
        return size;

    // Stretched mode: we don't care about the size of the image
    if (scalingType == ImageScalingType::STRETCH)
    {
        return size;
    }
    // Fit scaling mode: scale the view according to image ratio
    else if (scalingType == ImageScalingType::FIT && ntz(height) > 0)
    {
        float imageAspectRatio = originalWidth / originalHeight;

        // Grow height as much as possible then deduce width
        if (heightMode != YGMeasureModeUndefined)
        {
            if (ntz(width) > 0)
            {
                float viewAspectRatio = width / height;
                if (viewAspectRatio > imageAspectRatio)
                {
                    size.height = height;
                    size.width  = height * imageAspectRatio;
                }
                else
                {
                    size.width  = width;
                    size.height = width / imageAspectRatio;
                }
            }
            else
            {
                size.height = measureHeight(node, width, widthMode, height, heightMode, originalHeight, scalingType);
                size.width  = measureWidth(node, width, widthMode, height, heightMode, size.height * imageAspectRatio, scalingType);
            }
        }
        // Grow width as much as possible then deduce height
        else
        {
            size.width  = measureWidth(node, width, widthMode, height, heightMode, originalWidth, scalingType);
            size.height = measureHeight(node, width, widthMode, height, heightMode, size.width / imageAspectRatio, scalingType);
        }
    }
    // Crop (and fallback) method: grow as much as possible in both directions
    else
    {
        size.width  = measureWidth(node, width, widthMode, height, heightMode, originalWidth, scalingType);
        size.height = measureHeight(node, width, widthMode, height, heightMode, originalHeight, scalingType);
    }

    return size;
}

Image::Image()
{
    YGNodeSetMeasureFunc(this->ygNode, [](YGNodeConstRef node, float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode)
        {
            // Yoga reads an undefined or negative size as 0, with a warning
            YGSize size = imageMeasureFunc(node, width, widthMode, height, heightMode);
            return YGSize { std::isnan(size.width) ? 0.0f : std::max(size.width, 0.0f), std::isnan(size.height) ? 0.0f : std::max(size.height, 0.0f) }; });

    // This view uses a custom measure function, so the node type is automatically set to YGNodeTypeText,
    // which causes some deviations in the calculation of YGRoundToPixelGrid.
    // Another solution is to call `defaultConfig->setPointScaleFactor(factor)` in application.cpp.
    // (factor can be 0.0f or a larger value.)
    YGNodeSetNodeType(this->ygNode, YGNodeTypeDefault);

    AURORE_REGISTER_ENUM_XML_ATTRIBUTE(
        "scalingType", ImageScalingType, this->setScalingType,
        {
            { "fit", ImageScalingType::FIT },
            { "fill", ImageScalingType::FILL },
            { "stretch", ImageScalingType::STRETCH },
            { "center", ImageScalingType::CENTER },
        });

    AURORE_REGISTER_ENUM_XML_ATTRIBUTE(
        "imageAlign", ImageAlignment, this->setImageAlign,
        {
            { "top", ImageAlignment::TOP },
            { "right", ImageAlignment::RIGHT },
            { "bottom", ImageAlignment::BOTTOM },
            { "left", ImageAlignment::LEFT },
            { "center", ImageAlignment::CENTER },
        });

    AURORE_REGISTER_ENUM_XML_ATTRIBUTE(
        "interpolation", ImageInterpolation, this->setInterpolation,
        {
            { "linear", ImageInterpolation::LINEAR },
            { "nearest", ImageInterpolation::NEAREST },
        });

    this->registerFilePathXMLAttribute("image", [this](const std::string& value)
        { this->setImageFromFile(value); }

    );

    this->registerStringXMLAttribute("themeImage", [this](const std::string& value)
        { this->setThemeImage(value); });

    setClipsToBounds(true);
}

void Image::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    if (this->texture == 0)
        return;

    float coordX = x + this->imageX;
    float coordY = y + this->imageY;

    this->paint.xform[4] = coordX;
    this->paint.xform[5] = coordY;

    nvgBeginPath(vg);
    if (getClipsToBounds())
    {
        nvgRoundedRect(vg, x, y, width, height, getCornerRadius());
    }
    else
    {
        nvgRoundedRect(vg, coordX, coordY, this->imageWidth, this->imageHeight, getCornerRadius());
    }
    nvgFillPaint(vg, a(this->paint));
    nvgFill(vg);
}

void Image::onLayout()
{
    this->invalidateImageBounds();
}

void Image::setImageAlign(ImageAlignment align)
{
    this->align = align;
    this->invalidateImageBounds();
}

// Offset of the image along the axis that has spare (FIT) or overflowing
// (FILL) space, honoring the view's alignment. CENTER crops/letterboxes
// symmetrically, the axis matching edge pins the visible edge.
static float imageAxisOffset(float total, float inner, ImageAlignment align, bool horizontal)
{
    float spare = total - inner;
    if (horizontal)
    {
        if (align == ImageAlignment::LEFT)
            return 0.0F;
        if (align == ImageAlignment::RIGHT)
            return spare;
    }
    else
    {
        if (align == ImageAlignment::TOP)
            return 0.0F;
        if (align == ImageAlignment::BOTTOM)
            return spare;
    }
    return spare / 2.0F;
}

void Image::invalidateImageBounds()
{
    if (this->texture == 0)
        return;

    float width  = this->getWidth();
    float height = this->getHeight();

    float viewAspectRatio  = height / width;
    float imageAspectRatio = this->originalImageHeight / this->originalImageWidth;

    switch (this->scalingType)
    {
        case ImageScalingType::FIT:
        {
            if (viewAspectRatio <= imageAspectRatio)
            {
                this->imageHeight = this->getHeight();
                this->imageWidth  = this->imageHeight / imageAspectRatio;
                this->imageX      = imageAxisOffset(width, this->imageWidth, this->align, true);
                this->imageY      = 0;
            }
            else
            {
                this->imageWidth  = this->getWidth();
                this->imageHeight = this->imageWidth * imageAspectRatio;
                this->imageY      = imageAxisOffset(height, this->imageHeight, this->align, false);
                this->imageX      = 0;
            }
            break;
        }
        case ImageScalingType::FILL:
        {
            if (viewAspectRatio >= imageAspectRatio)
            {
                this->imageHeight = this->getHeight();
                this->imageWidth  = this->imageHeight / imageAspectRatio;
                this->imageX      = imageAxisOffset(width, this->imageWidth, this->align, true);
                this->imageY      = 0;
            }
            else
            {
                this->imageWidth  = this->getWidth();
                this->imageHeight = this->imageWidth * imageAspectRatio;
                this->imageY      = imageAxisOffset(height, this->imageHeight, this->align, false);
                this->imageX      = 0;
            }
            break;
        }
        case ImageScalingType::STRETCH:
            this->imageX      = 0;
            this->imageY      = 0;
            this->imageWidth  = this->getWidth();
            this->imageHeight = this->getHeight();
            break;
        case ImageScalingType::CENTER:
            this->imageHeight = this->originalImageHeight;
            this->imageWidth  = this->originalImageWidth;
            this->imageX      = (width - this->imageWidth) / 2.0F;
            this->imageY      = (height - this->imageHeight) / 2.0F;
            break;
        default:
            fatal("Unimplemented Image scaling type");
    }

    // Create the paint - actual X and Y positions are updated every frame in draw() to apply translation (scrolling...)
    NVGcontext* vg = Application::getNVGContext();
    this->paint    = nvgImagePattern(vg, 0, 0, this->imageWidth, this->imageHeight, 0, this->texture, 1.0f);
}

size_t Image::checkCache(const std::string& path)
{
    int tex = aurore::TextureCache::instance().getCache(path);
    if (tex > 0)
    {
        aurore::Logger::verbose("cache hit: {} {}", path, tex);
        this->innerSetImage(tex);
        return tex;
    }

    return 0;
}

void Image::setImageFromRes(const std::string& path)
{
    this->asyncGeneration++;
#ifdef USE_LIBROMFS
    // TextureCache decides when the texture is deleted (the previous one is
    // released by innerSetImage as it was held, before the flag changes)
    if (checkCache("@res/" + path) > 0)
    {
        this->setFreeTexture(false);
        return;
    }
    auto image = romfs::get(path);
    int tex    = nvgCreateImageMem(Application::getNVGContext(), this->getImageFlags(), (unsigned char*)image.data(), (int)image.size());
    if (tex == 0)
        return;
    innerSetImage(tex);
    this->setFreeTexture(false);
    TextureCache::instance().addCache("@res/" + path, tex);
#else
    this->setImageFromFile(std::string(AURORE_RESOURCES) + path);
#endif
}

void Image::setInterpolation(ImageInterpolation interpolation)
{
    this->interpolation = interpolation;
}

int Image::getImageFlags()
{
    if (this->interpolation == ImageInterpolation::NEAREST)
        return NVG_IMAGE_NEAREST;

    return 0;
}

void Image::setImageFromFile(const std::string& path)
{
    this->asyncGeneration++;
#ifdef USE_LIBROMFS
    if (path.rfind("@res/", 0) == 0)
        return this->setImageFromRes(path.substr(5));
#endif
    // TextureCache decides when the texture is deleted (the previous one is
    // released by innerSetImage as it was held, before the flag changes)
    if (checkCache(path) > 0)
    {
        this->setFreeTexture(false);
        return;
    }

    // Load texture
    int tex = nvgCreateImage(Application::getNVGContext(), path.c_str(), this->getImageFlags());
    if (tex == 0)
    {
        Logger::error("Cannot load image: {}", path);
        return;
    }
    innerSetImage(tex);
    this->setFreeTexture(false);

    // Save cache
    TextureCache::instance().addCache(path, tex);
}

void Image::setImageFromMem(const unsigned char* data, int size)
{
    this->asyncGeneration++;
    NVGcontext* vg = Application::getNVGContext();

    // Load texture: this image owns it (nothing else would delete it)
    int tex = nvgCreateImageMem(vg, this->getImageFlags(), const_cast<unsigned char*>(data), size);
    if (tex == 0)
    {
        Logger::error("Cannot load image from memory ({} bytes)", size);
        return;
    }
    innerSetImage(tex);
    this->setFreeTexture(true);
}

void Image::setImageFromRGBA(const unsigned char* pixels, int width, int height)
{
    this->asyncGeneration++;
    NVGcontext* vg = Application::getNVGContext();

    // Its own texture, of the same size: updated in place
    if (this->texture != 0 && this->freeTexture && (int)this->originalImageWidth == width && (int)this->originalImageHeight == height)
    {
        nvgUpdateImage(vg, this->texture, pixels);
        return;
    }

    int tex = nvgCreateImageRGBA(vg, width, height, this->getImageFlags(), pixels);
    if (tex == 0)
    {
        Logger::error("Cannot create a {}×{} image", width, height);
        return;
    }
    innerSetImage(tex);
    this->setFreeTexture(true);
}

void Image::setImageFromFileAsync(const std::string& path)
{
#ifdef USE_LIBROMFS
    if (path.rfind("@res/", 0) == 0)
        return this->setImageFromRes(path.substr(5));
#endif
    if (checkCache(path) > 0)
    {
        this->asyncGeneration++;
        this->setFreeTexture(false);
        return;
    }

    unsigned generation = ++this->asyncGeneration;
    int flags           = this->getImageFlags();
    Threading::asyncDecode([this, alive = aliveToken(), path, generation, flags]()
        {
            int width = 0, height = 0;
            // Freed whatever happens to the image meanwhile
            std::shared_ptr<unsigned char> pixels(nvgDecodeImage(path.c_str(), &width, &height), nvgFreeImageData);
            aurore::sync([this, alive, path, generation, flags, pixels, width, height]()
                {
                    if (alive.expired())
                        return;
                    if (generation != this->asyncGeneration)
                        return; // set again since
                    if (!pixels)
                    {
                        Logger::error("Cannot load image: {}", path);
                        return;
                    }
                    // Loaded by another image meanwhile
                    if (checkCache(path) > 0)
                    {
                        this->setFreeTexture(false);
                        return;
                    }
                    int tex = nvgCreateImageRGBA(Application::getNVGContext(), width, height, flags, pixels.get());
                    if (tex == 0)
                        return;
                    innerSetImage(tex);
                    this->setFreeTexture(false);
                    TextureCache::instance().addCache(path, tex);
                }); });
}

void Image::setThemeImage(const std::string& key)
{
    this->asyncGeneration++;
    if (key.empty())
    {
        this->stopFollowingTheme();
        return;
    }
    this->themeImageKey = key;
    if (!this->followingTheme)
    {
        this->followingTheme      = true;
        this->themeToken          = std::make_shared<char>();
        std::weak_ptr<char> token = this->themeToken;
        this->themeChanged        = Application::getThemeChangedEvent()->subscribe([this, token](ThemeVariant)
            {
                if (!token.expired())
                    this->refreshThemeImage(); });
        this->themeImageLoaded    = ThemeImages::getLoadedEvent()->subscribe([this, token](const ThemeImage* image)
            {
                if (!token.expired() && (image == this->themeImagePending.get() || image == this->themeImage.get()))
                    this->refreshThemeImage(); });
    }
    this->refreshThemeImage();
}

void Image::stopFollowingTheme()
{
    if (this->followingTheme)
    {
        Application::getThemeChangedEvent()->unsubscribe(this->themeChanged);
        ThemeImages::getLoadedEvent()->unsubscribe(this->themeImageLoaded);
        this->followingTheme = false;
        this->themeToken.reset();
    }
    this->themeImageKey.clear();
    this->themeImagePending.reset();
}

void Image::refreshThemeImage()
{
    std::shared_ptr<ThemeImage> image = Theme::image(this->themeImageKey);
    if (image && image->state == ThemeImage::State::LOADING)
    {
        // The picture shown stays until it is ready
        this->themeImagePending = image;
        return;
    }
    this->themeImagePending.reset();
    if (image && image == this->themeImage)
        return;
    if (!image || !image->ready())
    {
        // Neither the theme nor the app has one
        if (this->themeImage)
        {
            this->releaseTexture();
            this->invalidate();
        }
        return;
    }
    this->settingThemeImage = true;
    this->innerSetImage(image->texture);
    this->settingThemeImage = false;
    this->setFreeTexture(false);
    this->themeImage = image;
}

void Image::releaseTexture()
{
    // A theme's picture is the theme's: let go of it, not deleted
    if (this->themeImage)
    {
        if (this->texture == this->themeImage->texture)
            this->texture = 0;
        this->themeImage.reset();
    }
    if (this->texture == 0)
        return;

    // An owned texture is deleted; one from TextureCache (or the app's) is
    // handed back to the cache, which ignores a texture it does not hold
    if (this->freeTexture)
        nvgDeleteImage(Application::getNVGContext(), this->texture);
    else
        TextureCache::instance().removeCache(this->texture);

    this->texture = 0;
}

void Image::setImageAsync(std::function<void(std::function<void(const std::string&, size_t length)>)> cb)
{
    // The callback may be called more than once, or after the image is gone
    cb([this, alive = aliveToken()](const std::string& data, size_t length)
        { aurore::sync([this, alive, data, length]()
              {
                  if (alive.expired())
                      return;
                  size_t size = std::min(length, data.size());
                  if (size == 0 || size > (size_t)INT_MAX)
                      return;
                  this->setImageFromMem((unsigned char*)data.data(), (int)size); }); });
}

void Image::innerSetImage(int tex)
{
    if (tex == 0)
    {
        Logger::error("Cannot set texture: 0");
        return;
    }

    NVGcontext* vg = Application::getNVGContext();

    // Set another way: the theme's slot is not followed any more
    if (!this->settingThemeImage && !this->themeImageKey.empty())
        this->stopFollowingTheme();

    // Release the old texture as it was held (owned or cached); the same
    // texture again (a cache hit on the path already shown) is kept
    if (this->texture != tex)
        this->releaseTexture();
    else if (!this->freeTexture)
        TextureCache::instance().removeCache(this->texture); // the hit counted it once more

    // Set the new texture
    this->texture = tex;

    int width, height;
    nvgImageSize(vg, this->texture, &width, &height);
    this->originalImageWidth  = (float)width;
    this->originalImageHeight = (float)height;

    this->invalidate();
}

void Image::clear()
{
    this->asyncGeneration++;
    this->stopFollowingTheme();
    // A cached texture is handed back too: it was never released, so the
    // cache could never free it
    this->releaseTexture();
}

void Image::setScalingType(ImageScalingType scalingType)
{
    this->scalingType = scalingType;

    this->invalidate();
}

ImageScalingType Image::getScalingType()
{
    return this->scalingType;
}

float Image::getOriginalImageWidth()
{
    return this->originalImageWidth;
}

int Image::getTexture()
{
    return this->texture;
}

void Image::setFreeTexture(bool value)
{
    this->freeTexture = value;
}

bool Image::getFreeTexture()
{
    return this->freeTexture;
}

float Image::getOriginalImageHeight()
{
    return this->originalImageHeight;
}

Image::~Image()
{
    this->stopFollowingTheme();
    this->releaseTexture();
}

View* Image::create()
{
    return new Image();
}

} // namespace aurore

/*
    Copyright 2019 WerWolv
    Copyright 2019 p-sam
    Copyright 2020-2021 natinusala

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

#pragma once

#include <memory>
#include <aurore/core/event.hpp>
#include <aurore/core/theme.hpp>
#include <aurore/core/view.hpp>

namespace aurore
{

/// This dictates what to do with the image if there is not
/// enough room for the view to grow and display the whole image,
/// or if the view is bigger than the image
enum class ImageScalingType
{
    /// The image is scaled to fit the view boundaries, aspect ratio is conserved
    FIT,
    /// The image is scaled to fill the view boundaries, aspect ratio is conserved
    FILL,
    /// The image is stretched to fit the view boundaries (aspect ratio is not conserved). The original image dimensions are entirely ignored in the layout process.
    STRETCH,
    /// The image is either cropped (not enough space) or untouched (too much space)
    CENTER,
};

/// This dictates what interpolation to use when down / up scaling the image
enum class ImageInterpolation
{
    LINEAR,
    NEAREST
};

/// Alignment of the image inside the view for FIT and CROP scaling types
enum class ImageAlignment
{
    TOP,
    RIGHT,
    BOTTOM,
    LEFT,
    CENTER,
};

/// An image. The view will try to grow as much
/// as possible to fit the image. The scaling type dictates
/// what to do with the image if there is not enough or too much space
/// for the view compared to the image inside.
/// Supported formats are: JPG, PNG, TGA, BMP and GIF (not animated).
class Image : public View
{
  public:
    /// An empty image (FIT, centred, linear interpolation).
    Image();
    ~Image();

    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;
    void onLayout() override;

    /**
     * Sets the interpolation method for the image. Default is LINEAR.
     *
     * As the interpolation is set when the texture is created, and since we don't
     * want to always store the image buffer in the view, this only takes effect
     * after (re) loading the image using the setImage* methods.
     *
     * If you are using the interpolation XML attribute, you have to set it before the
     * actual image attribute.
     */
    void setInterpolation(ImageInterpolation interpolation);

    /**
     * Sets the image from the given resource name.
     *
     * See Image class documentation for the list of supported
     * image formats.
     */
    void setImageFromRes(const std::string& name);

    /**
     * Sets the image from the given file path.
     *
     * See Image class documentation for the list of supported
     * image formats.
     */
    void setImageFromFile(const std::string& path);

    /**
     * Sets the image from the given file path, decoded on the decode thread (Threading::asyncDecode):
     * the UI thread only uploads the pixels, so a grid of covers does not
     * stall while they load. The image stays as it was until then (a cached
     * texture is shown at once). An image set again meanwhile, or deleted,
     * drops the result.
     */
    void setImageFromFileAsync(const std::string& path);

    /**
     * Sets the image from memory.
     *
     * See Image class documentation for the list of supported
     * image formats.
     */
    void setImageFromMem(const unsigned char* data, int size);

    /**
     * Sets the image from raw RGBA pixels (width × height × 4 bytes), for an
     * image made by the app (a QR code, a video frame, a thumbnail it
     * decoded). Pixels of the same size as the image shown replace its own
     * texture's in place, without making a new one.
     */
    void setImageFromRGBA(const unsigned char* pixels, int width, int height);

    /**
     * Shows the picture of the image slot `key` (Theme::addImageSlot): the
     * theme's, else the app's fallback, and follows the theme as it changes
     * (the new picture shown once loaded; none: the image is cleared). XML:
     * themeImage="playguard/loading/art". Another setImage* call, or "",
     * stops following it.
     */
    void setThemeImage(const std::string& key);
    /// The image slot followed (setThemeImage()), "" for none.
    const std::string& getThemeImage() const { return this->themeImageKey; }

    /// Shows the nanovg image `texture` (what every setImage* ends with).
    virtual void innerSetImage(int texture);

    /// `cb` gets a function to call, from any thread, with the encoded image's bytes:
    /// the image is set from them on the main thread (nothing if the view is gone
    /// or `length` is 0).
    void setImageAsync(std::function<void(std::function<void(const std::string&, size_t length)>)> cb);

    /// Shows nothing (the texture freed when the image owns it).
    void clear();

    /**
     * Sets the scaling type for this image.
     *
     * Default is FIT.
     */
    void setScalingType(ImageScalingType scalingType);

    /// See setScalingType().
    ImageScalingType getScalingType();

    /**
     * Sets the alignment of the image inside the view.
     * Only used if scaling it set to FIT when the aspect ratio is different
     * or CROP, to change the region of the image to draw.
     *
     * Default is CENTER.
     */
    void setImageAlign(ImageAlignment align);

    /**
     * Whether to destroy the current texture before updating the image texture
     */
    void setFreeTexture(bool value);
    bool getFreeTexture();

    /// The nanovg image shown, 0 for none.
    int getTexture();
    /// The size of the picture shown, in pixels.
    float getOriginalImageWidth();
    float getOriginalImageHeight();

    /// Makes an image for XML (`<brls:Image>`).
    static View* create();

  protected:
    ImageScalingType scalingType     = ImageScalingType::FIT;
    ImageAlignment align             = ImageAlignment::CENTER;
    ImageInterpolation interpolation = ImageInterpolation::LINEAR;

    int texture = 0;

    NVGpaint paint;

    void invalidateImageBounds();
    int getImageFlags();
    size_t checkCache(const std::string& path);

    float originalImageWidth  = 0;
    float originalImageHeight = 0;

    float imageX      = 0;
    float imageY      = 0;
    float imageHeight = 0;
    float imageWidth  = 0;

    bool freeTexture = true;

    /// Bumped each time the image is set: an async load set before is stale
    unsigned asyncGeneration = 0;

    /// Deletes the texture if owned, or hands it back to TextureCache
    void releaseTexture();

  private:
    /// setThemeImage: the slot followed, the picture shown, the one loading
    std::string themeImageKey;
    std::shared_ptr<ThemeImage> themeImage;
    std::shared_ptr<ThemeImage> themeImagePending;
    bool settingThemeImage = false;
    bool followingTheme    = false;
    /// Gone with the view: an event firing from a copy of its callbacks
    /// does not call a view deleted meanwhile
    std::shared_ptr<char> themeToken;
    Event<ThemeVariant>::Subscription themeChanged;
    Event<const ThemeImage*>::Subscription themeImageLoaded;
    void refreshThemeImage();
    void stopFollowingTheme();
};

} // namespace aurore

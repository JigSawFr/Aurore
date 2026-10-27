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

#pragma once

#include <nanovg.h>

#include <borealis/core/event.hpp>
#include <functional>
#include <memory>
#include <string>

namespace brls
{

/**
 * A picture a theme file names (its background_image; what decorates it,
 * its logos): a texture made from it, at most MAX_WIDTH x MAX_HEIGHT.
 * Only the UI thread reads or changes it.
 */
struct ThemeImage
{
    enum class State
    {
        LOADING,
        READY,
        FAILED, // cannot be read or decoded: draws nothing
    };

    std::string path; // as Theme::resolveAsset() made it
    int flags   = 0;  // NVG_IMAGE_* it was made with
    State state = State::LOADING;
    int texture = 0; // 0 until READY
    int width = 0, height = 0; // the texture's, after downscaling
    NVGcolor mean {};          // its average colour, opaque, once READY

    bool ready() const { return this->state == State::READY; }

    ThemeImage() = default;
    ThemeImage(const ThemeImage&) = delete;
    ThemeImage& operator=(const ThemeImage&) = delete;
    // Deletes the texture: the last owner lets it go on the UI thread
    ~ThemeImage();
};

/**
 * The one loader of theme pictures. load() hands back the image at once,
 * still loading: it is decoded on the async thread (brls::async), made
 * smaller when larger than MAX_WIDTH x MAX_HEIGHT (its ratio kept), and its
 * texture is made on the UI thread (brls::sync), which draws again. A file
 * that cannot be read is logged once and stays FAILED.
 *
 * The images are shared by path and flags while someone holds them, and
 * freed when the last holder lets go: whoever draws a theme's picture keeps
 * the shared_ptr while that theme is in use (Application does for the
 * background), and drops it when the theme changes.
 *
 * PNG and JPEG (what stb_image decodes; not WebP). Paths are files,
 * resources ("@res/...") or romfs files.
 */
class ThemeImages
{
  public:
    static constexpr int MAX_WIDTH  = 1920;
    static constexpr int MAX_HEIGHT = 1080;

    /**
     * The image at `path`, made with `flags` (NVG_IMAGE_REPEATX...):
     * loading, or ready when someone holds it already. From the UI thread.
     */
    static std::shared_ptr<ThemeImage> load(const std::string& path, int flags = 0);

    /**
     * Fired on the UI thread when an image loaded with load() is ready or
     * failed (its texture made), before the frame that can draw it.
     */
    static Event<const ThemeImage*>* getLoadedEvent();

    /**
     * The average colour of the picture at `path` (opaque), for the
     * contrast checks: remembered once known (a load() computes it on the
     * async thread); else decoded here, on the calling thread: not from the
     * UI thread (a check, a test). False when the file cannot be read.
     */
    static bool meanColor(const std::string& path, NVGcolor* color);

    /**
     * The same, only when already known: never decodes.
     */
    static bool knownMeanColor(const std::string& path, NVGcolor* color);

    /**
     * Whether there is a file at `path` (a resource, or a file on disk),
     * without reading it.
     */
    static bool exists(const std::string& path);

    /**
     * How many pictures the async thread is decoding now. A test waits for
     * none, in real time, before it counts frames: the frames drawn while a
     * picture decodes then do not depend on the machine's speed.
     */
    static int decoding();

    /**
     * Deletes every texture still held, before the NVG context goes away
     * (Application::exit): an image held afterwards is FAILED, empty.
     */
    static void releaseAll();
};

} // namespace brls

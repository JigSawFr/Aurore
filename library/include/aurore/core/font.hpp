// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <unordered_map>
#include <aurore/core/assets.hpp>

namespace aurore
{

static constexpr const int FONT_INVALID = -1;

inline constexpr const char* FONT_REGULAR                = "regular"; // regular Latin font
inline constexpr const char* FONT_KOREAN_REGULAR         = "korean"; // regular Korean font
inline constexpr const char* FONT_CHINESE_SIMPLIFIED     = "zh-Hans"; // Simplified Chinese
inline constexpr const char* FONT_CHINESE_SIMPLIFIED_EXT = "zh-Hans-ext"; // Simplified Chinese Extended
inline constexpr const char* FONT_CHINESE_TRADITIONAL    = "zh-Hant"; // Traditional Chinese
inline constexpr const char* FONT_JAPANESE               = "ja"; // Japanese (where the regular font has no kana nor kanji)
inline constexpr const char* FONT_MATERIAL_ICONS         = "material"; // Material icons font
inline constexpr const char* FONT_SWITCH_ICONS           = "switch"; // the controller buttons, at the code points of the Switch's shared font (U+E0E0 for A...)
inline constexpr const char* FONT_EMOJI                  = "emoji";

typedef std::unordered_map<std::string, int> FontStash;

// Platform interface to load fonts from disk or other sources (system / shared font...)
class FontLoader
{
  public:
    inline static std::string USER_FONT_PATH = AURORE_ASSET("font/font.ttf");
    inline static std::string USER_ICON_PATH = AURORE_ASSET("font/icon.ttf");
    inline static std::string USER_EMOJI_PATH;

    virtual ~FontLoader() { }
    /**
     * Called once on init to load every font in the font stash.
     *
     * The implementation must use the Application::loadFont and
     * Application::loadFontFromMemory methods to load as much as possible
     * of the "built-in" fonts defined in the FONT_* constants above.
     */
    virtual void loadFonts() = 0;

  protected:
    /**
     * Convenience method to load a font from a file path
     * with some more logging.
     */
    bool loadFontFromFile(std::string fontName, std::string filePath);

    /**
     * Can be called internally to load the Material icons font from resources.
     * Returns true if the operation succeeds.
     */
    bool loadMaterialFromResources();
};

} // namespace aurore

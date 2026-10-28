/*
    Copyright 2021 natinusala
    Copyright 2023 xfangfang

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

#include <string>
#include <vector>
#include <borealis/core/font.hpp>

namespace brls
{

// Font loader of every platform but the Switch: the text and the buttons
// from the library's resources (font/aurore_text.ttf, font/aurore_buttons.ttf,
// or the app's font/font.ttf and font/icon.ttf), and Chinese, Japanese and
// Korean from the app's font/cjk.ttc (or .ttf, .otf) or else from the system
// (see docs/fonts.md)
class DesktopFontLoader : public FontLoader
{
  public:
    void loadFonts() override;

    /**
     * Whether the system's fonts are looked up (Chinese, Japanese and Korean;
     * the colour emoji with FreeType). The test harness turns it off: its
     * pictures must not depend on the fonts of the machine.
     */
    inline static bool useSystemFonts = true;

    // A font file, and the face in it (a .ttc holds several)
    struct FontFile
    {
        std::string path;
        int index = 0;
    };

    /**
     * The system's font for a language ("ja", "ko", "zh-Hans", "zh-Hant"):
     * fontconfig's choice on Linux, CoreText's on macOS and iOS, a known
     * file on Windows and Android. An empty path when there is none.
     */
    static FontFile findSystemFont(const std::string& language);

  private:
    bool loadFont(const std::string& name, const std::string& path);
    bool loadFontFile(const std::string& name, const FontFile& file);
    bool loadAppCjkFont(NVGcontext* vg);
    void loadSystemCjkFonts(NVGcontext* vg);
};

} // namespace brls

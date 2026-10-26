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

// The theme files built into Aurore ("builtin:<id>", Theme::builtinThemes):
// every app has them without shipping files. Each one passes the contrast
// check (Theme::checkContrast). Their names are the strings of
// resources/i18n/*/themes.json. The format: docs/themes.md.

#include "theme_presets.hpp"

namespace brls
{

const std::vector<BuiltinTheme>& builtinThemeTexts()
{
    static const std::vector<BuiltinTheme> themes = {
        { "high-contrast-dark", R"theme(
name = @i18n/themes/presets/high_contrast_dark
author = Aurore
version = 1
description = White on black, a yellow focus: the most legible dark look.
inherit = dark

background = #000000
surface = #000000
text = #ffffff
accent = #ffd60a
warning = #ffb000
danger = #ff6b6b

[colors]
cadran/surf2 = #141414
cadran/text2 = #ffffff
cadran/text3 = #e6e6e6
brls/text_disabled = #a6a6a6
brls/header/subtitle = #e6e6e6
brls/list/listItem_value_color = #ffffff
cadran/line = rgba(255, 255, 255, 0.55)
brls/sidebar/separator = rgba(255, 255, 255, 0.55)
brls/applet_frame/separator = rgba(255, 255, 255, 0.55)
brls/header/border = rgba(255, 255, 255, 0.55)
cadran/border = #ffffff
brls/button/enabled_border_color = #ffffff
brls/highlight/color1 = #ffd60a
brls/highlight/color2 = #ffd60a
cadran/hl1 = #ffd60a
cadran/hl2 = #ffd60a
brls/backdrop = rgba(0, 0, 0, 0.9)

[metrics]
brls/highlight/stroke_width = 6
)theme" },

        { "high-contrast-light", R"theme(
name = @i18n/themes/presets/high_contrast_light
author = Aurore
version = 1
description = Black on white, a deep blue focus: the most legible light look.
inherit = light

background = #ffffff
surface = #ffffff
text = #000000
accent = #0039b3
warning = #8a4b00
danger = #b00020

[colors]
cadran/surf2 = #f0f0f0
cadran/text2 = #000000
cadran/text3 = #1f1f1f
brls/text_disabled = #5c5c5c
brls/header/subtitle = #1f1f1f
brls/list/listItem_value_color = #000000
cadran/line = rgba(0, 0, 0, 0.6)
brls/sidebar/separator = rgba(0, 0, 0, 0.6)
brls/applet_frame/separator = rgba(0, 0, 0, 0.6)
brls/header/border = rgba(0, 0, 0, 0.6)
cadran/border = #000000
brls/button/enabled_border_color = #000000
brls/highlight/color1 = #0039b3
brls/highlight/color2 = #0039b3
cadran/hl1 = #0039b3
cadran/hl2 = #0039b3

[metrics]
brls/highlight/stroke_width = 6
)theme" },

        { "ocean", R"theme(
name = @i18n/themes/presets/ocean
author = Aurore
version = 1
description = Deep blue, a sky-blue accent.
inherit = dark

background = #0a1a2f
surface = #10263f
text = #eaf2fb
accent = #3fa7ff
)theme" },

        { "amethyst", R"theme(
name = @i18n/themes/presets/amethyst
author = Aurore
version = 1
description = Violet, light or dark as the system is.

[light]
background = #f3f0f8
surface = #ffffff
text = #1d1530
accent = #6b3fc9

[dark]
background = #17112a
surface = #211938
text = #f1ecfa
accent = #a98bff
)theme" },

        { "sand", R"theme(
name = @i18n/themes/presets/sand
author = Aurore
version = 1
description = Warm paper, a terracotta accent.
inherit = light

background = #f4eee4
surface = #fffaf2
text = #2b2118
accent = #b34a2b
)theme" },
    };
    return themes;
}

} // namespace brls

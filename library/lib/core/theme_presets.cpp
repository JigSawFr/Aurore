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

#include <iterator>

namespace brls
{

const std::vector<BuiltinTheme>& builtinThemeTexts()
{
    static const std::vector<BuiltinTheme> themes = {
        { "high-contrast-dark", R"theme(
name = @i18n/themes/presets/high_contrast_dark
author = Aurore
icon = contrast
version = 1
description = White on black, a yellow focus: the most legible dark look.
inherit = dark
counterpart = high-contrast-light

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
icon = contrast
version = 1
description = Black on white, a deep blue focus: the most legible light look.
inherit = light
counterpart = high-contrast-dark

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
icon = wave
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
icon = gem
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
icon = dune
version = 1
description = Warm paper, a terracotta accent.
inherit = light

background = #f4eee4
surface = #fffaf2
text = #2b2118
accent = #b34a2b
)theme" },

        // The seasons (builtin:seasonal picks one by date, below). Each has
        // both variants and keeps the core's roles: the accent for what is
        // chosen, warning amber, danger red; the season is in the
        // surfaces, the focus ring's two colours, the glows, the bars, the
        // tiles' and avatars' inks.

        { "winter", R"theme(
name = @i18n/themes/presets/winter
author = Aurore
icon = snowflake
particles = snow
version = 1
description = Frost and silver: ice-blue on a cold slate night, or glacier blue on frost.

[light]
background = #edf3f8
surface = #ffffff
text = #10202f
accent = #2a6496
warning = #8a5200
danger = #b42335
cadran/blue = #1f5c9c
cadran/violet = #5747b8
cadran/pink = #a63d70
brls/highlight/color1 = #2a6496
brls/highlight/color2 = #6a8fb5
cadran/hl1 = #2a6496
cadran/hl2 = #6a8fb5
cadran/bar = #cfdeec
cadran/bar_today = #88a9c9
cadran/glow = rgba(120, 170, 220, 0.18)
cadran/glow_strong = rgba(120, 170, 220, 0.26)

[dark]
background = #0f1620
surface = #17202d
text = #eef4fb
accent = #9cc9ff
warning = #ffc46b
danger = #ff7a85
cadran/blue = #7fd4ff
cadran/violet = #b9b0ff
cadran/pink = #f4a6c8
brls/highlight/color1 = #9cc9ff
brls/highlight/color2 = #f2f8ff
cadran/hl1 = #9cc9ff
cadran/hl2 = #f2f8ff
cadran/bar = #243246
cadran/bar_today = #3f5675
cadran/glow = rgba(156, 201, 255, 0.10)
cadran/glow_strong = rgba(156, 201, 255, 0.18)
cadran/brand/track = #1f2b3b
cadran/brand/fill = #9cc9ff
cadran/brand/glow = #f2f8ff
brls/backdrop = rgba(6, 10, 16, 0.88)

[colors]
cadran/avatar0 = #9cc9ff
cadran/avatar1 = #dfe8f5
cadran/avatar2 = #b9b0ff
cadran/avatar3 = #7fd4ff
)theme" },

        { "valentine", R"theme(
name = @i18n/themes/presets/valentine
author = Aurore
icon = heart
particles = hearts
version = 1
description = Rose and wine: a crimson accent, a focus pulsing from red to blush.

[light]
background = #fbeff2
surface = #fffafb
text = #3a0f1f
accent = #c1121f
warning = #8a5200
danger = #9d1d5a
cadran/blue = #4a5bc4
cadran/violet = #8a3fa8
cadran/pink = #b8285e
brls/highlight/color1 = #c1121f
brls/highlight/color2 = #d0577f
cadran/hl1 = #c1121f
cadran/hl2 = #d0577f
cadran/bar = #f2cdd7
cadran/bar_today = #e07c98
cadran/glow = rgba(255, 92, 138, 0.14)
cadran/glow_strong = rgba(255, 92, 138, 0.22)

[dark]
background = #1f0a14
surface = #2c1020
text = #fdeef2
accent = #ff5c8a
warning = #ffc46b
danger = #ff8a5c
cadran/blue = #b9a6ff
cadran/violet = #e0a6ff
cadran/pink = #ff9ec0
brls/highlight/color1 = #ff5c8a
brls/highlight/color2 = #ffc2d4
cadran/hl1 = #ff5c8a
cadran/hl2 = #ffc2d4
cadran/bar = #4a1a30
cadran/bar_today = #8a2f58
cadran/glow = rgba(255, 92, 138, 0.13)
cadran/glow_strong = rgba(255, 92, 138, 0.22)
cadran/brand/track = #3a1424
cadran/brand/fill = #ff5c8a
cadran/brand/glow = #ffc2d4
brls/backdrop = rgba(16, 4, 10, 0.88)

[colors]
cadran/avatar0 = #ff7aa2
cadran/avatar1 = #ffc2d4
cadran/avatar2 = #e0a6ff
cadran/avatar3 = #ffd88a
)theme" },

        { "lunar-new-year", R"theme(
name = @i18n/themes/presets/lunar_new_year
author = Aurore
icon = lantern
particles = sparkles
version = 1
description = Red lacquer and gold, jade for the details, lanterns glowing behind.

[light]
background = #fbf2e4
surface = #fffaf0
text = #3b0c0c
accent = #b5121b
warning = #8a5200
danger = #8f1d4a
cadran/blue = #0f7a5a
cadran/violet = #6b3fa0
cadran/pink = #b3245a
brls/highlight/color1 = #b5121b
brls/highlight/color2 = #a87908
cadran/hl1 = #b5121b
cadran/hl2 = #a87908
cadran/bar = #f0d3c4
cadran/bar_today = #d9826e
cadran/glow = rgba(226, 58, 58, 0.12)
cadran/glow_strong = rgba(246, 196, 69, 0.30)
particles_color = #a87908, #c9971c

[dark]
background = #1f0a0b
particles_color = #f6c445, #ffe08a
surface = #2d1112
text = #fff1db
accent = #f6c445
warning = #ffa94d
danger = #ff7a6e
cadran/blue = #5fd3a8
cadran/violet = #d6a6ff
cadran/pink = #ff8c9e
brls/highlight/color1 = #f6c445
brls/highlight/color2 = #ff6b5b
cadran/hl1 = #f6c445
cadran/hl2 = #ff6b5b
cadran/bar = #4a1a1b
cadran/bar_today = #8a2f2c
cadran/glow = rgba(226, 58, 58, 0.20)
cadran/glow_strong = rgba(246, 196, 69, 0.20)
cadran/brand/track = #3a1414
cadran/brand/fill = #e23a3a
cadran/brand/glow = #f6c445
brls/backdrop = rgba(14, 3, 4, 0.88)

[colors]
cadran/avatar0 = #ff6b5b
cadran/avatar1 = #f6c445
cadran/avatar2 = #5fd3a8
cadran/avatar3 = #ff8c9e
)theme" },

        { "spring", R"theme(
name = @i18n/themes/presets/spring
author = Aurore
icon = blossom
particles = petals
version = 1
description = Cherry blossoms: blush and leaf green by day, pink petals at night.

[light]
background = #fbf0f3
surface = #ffffff
text = #3b1e2c
accent = #b8487a
warning = #8a5200
danger = #c0262d
cadran/blue = #3c7a37
cadran/violet = #7a4fb0
cadran/pink = #b8487a
brls/highlight/color1 = #b8487a
brls/highlight/color2 = #4f8a45
cadran/hl1 = #b8487a
cadran/hl2 = #4f8a45
cadran/bar = #f3d0dc
cadran/bar_today = #e58fb0
cadran/glow = rgba(247, 168, 196, 0.22)
cadran/glow_strong = rgba(247, 168, 196, 0.34)

[dark]
background = #1c1320
surface = #281b2d
text = #fcedf3
accent = #ff9cc4
warning = #ffc46b
danger = #ff7f50
cadran/blue = #9ed98a
cadran/violet = #c7a6ff
cadran/pink = #ffb3d1
brls/highlight/color1 = #ff9cc4
brls/highlight/color2 = #ffe3ee
cadran/hl1 = #ff9cc4
cadran/hl2 = #ffe3ee
cadran/bar = #3d2742
cadran/bar_today = #6e4677
cadran/glow = rgba(255, 156, 196, 0.10)
cadran/glow_strong = rgba(255, 156, 196, 0.18)
cadran/brand/track = #33223a
cadran/brand/fill = #ff9cc4
cadran/brand/glow = #ffe3ee
brls/backdrop = rgba(12, 7, 14, 0.88)

[colors]
cadran/avatar0 = #f7a8c4
cadran/avatar1 = #b8e0a0
cadran/avatar2 = #c9b3f2
cadran/avatar3 = #ffd7a8
)theme" },

        { "summer", R"theme(
name = @i18n/themes/presets/summer
author = Aurore
icon = sun
version = 1
description = Sun and lagoon by day, a sunset over a warm night.

[light]
background = #fdf7ea
surface = #ffffff
text = #0e2f3a
accent = #00798c
warning = #8f5400
danger = #c2362b
cadran/blue = #1b62b8
cadran/violet = #6c4bc4
cadran/pink = #b83c6a
brls/highlight/color1 = #c4500f
brls/highlight/color2 = #00798c
cadran/hl1 = #c4500f
cadran/hl2 = #00798c
cadran/bar = #cfe9ec
cadran/bar_today = #6fb8c3
cadran/glow = rgba(255, 179, 71, 0.20)
cadran/glow_strong = rgba(255, 179, 71, 0.30)

[dark]
background = #0d2327
surface = #133037
text = #fdf5e8
accent = #ff8f5a
warning = #ffd166
danger = #ff6b81
cadran/blue = #48d6cf
cadran/violet = #b49cff
cadran/pink = #ff9ec0
brls/highlight/color1 = #ff8f5a
brls/highlight/color2 = #ffd166
cadran/hl1 = #ff8f5a
cadran/hl2 = #ffd166
cadran/bar = #1d4248
cadran/bar_today = #2f6870
cadran/glow = rgba(255, 143, 90, 0.12)
cadran/glow_strong = rgba(255, 209, 102, 0.18)
cadran/brand/track = #18363b
cadran/brand/fill = #ff8f5a
cadran/brand/glow = #ffd166
brls/backdrop = rgba(4, 14, 16, 0.88)

[colors]
cadran/avatar0 = #ffb347
cadran/avatar1 = #4dd0e1
cadran/avatar2 = #ff8a65
cadran/avatar3 = #aed581
)theme" },

        { "autumn", R"theme(
name = @i18n/themes/presets/autumn
author = Aurore
icon = leaf
particles = leaves
version = 1
description = Maple and amber leaves on linen, or on dark bark.

[light]
background = #f6efe4
surface = #fffbf5
text = #2d1c12
accent = #a83f12
warning = #7f5500
danger = #b0203a
cadran/blue = #2e6f6a
cadran/violet = #7a4a8f
cadran/pink = #a8405a
brls/highlight/color1 = #a83f12
brls/highlight/color2 = #6b7a1a
cadran/hl1 = #a83f12
cadran/hl2 = #6b7a1a
cadran/bar = #ead9c4
cadran/bar_today = #c99366
cadran/glow = rgba(232, 116, 59, 0.14)
cadran/glow_strong = rgba(242, 184, 75, 0.26)

[dark]
background = #1a1310
surface = #251b16
text = #f6ece1
accent = #e8743b
warning = #f2b84b
danger = #ff6b6b
cadran/blue = #6fb7b0
cadran/violet = #c39bd3
cadran/pink = #f08a8a
brls/highlight/color1 = #e8743b
brls/highlight/color2 = #f2b84b
cadran/hl1 = #e8743b
cadran/hl2 = #f2b84b
cadran/bar = #3a2a20
cadran/bar_today = #6b4a33
cadran/glow = rgba(232, 116, 59, 0.12)
cadran/glow_strong = rgba(242, 184, 75, 0.16)
cadran/brand/track = #33251c
cadran/brand/fill = #e8743b
cadran/brand/glow = #f2b84b
brls/backdrop = rgba(10, 7, 5, 0.88)

[colors]
cadran/avatar0 = #e8743b
cadran/avatar1 = #f2b84b
cadran/avatar2 = #c7d36f
cadran/avatar3 = #d9a07a
)theme" },

        { "halloween", R"theme(
name = @i18n/themes/presets/halloween
author = Aurore
icon = pumpkin
particles = embers
version = 1
description = Pumpkin and candlelight on a violet night; by day, moonlit lilac.

[light]
background = #f1edf6
surface = #fbf9fe
text = #22162f
accent = #c2520a
warning = #7f5500
danger = #b3261e
cadran/blue = #2f7a1f
cadran/violet = #6a3fb5
cadran/pink = #a8306a
brls/highlight/color1 = #6a3fb5
brls/highlight/color2 = #c2520a
cadran/hl1 = #6a3fb5
cadran/hl2 = #c2520a
cadran/bar = #ddd0ec
cadran/bar_today = #a98bcf
cadran/glow = rgba(140, 82, 255, 0.12)
cadran/glow_strong = rgba(255, 138, 31, 0.22)

[dark]
background = #150d1f
surface = #21152f
text = #f3ead8
accent = #ff8a1f
warning = #ffd166
danger = #ff5d73
cadran/blue = #9ee37d
cadran/violet = #b794ff
cadran/pink = #ff9ecb
brls/highlight/color1 = #ff8a1f
brls/highlight/color2 = #ffd166
cadran/hl1 = #ff8a1f
cadran/hl2 = #ffd166
brls/highlight/shadow = #ff8a1f
cadran/bar = #3a2752
cadran/bar_today = #6d4a9c
cadran/glow = rgba(140, 82, 255, 0.16)
cadran/glow_strong = rgba(255, 138, 31, 0.18)
cadran/brand/track = #2c1d40
cadran/brand/fill = #ff8a1f
cadran/brand/glow = #ffd166
brls/backdrop = rgba(8, 4, 14, 0.9)

[colors]
cadran/avatar0 = #ff8a1f
cadran/avatar1 = #b794ff
cadran/avatar2 = #9ee37d
cadran/avatar3 = #f3ead8
)theme" },

        { "christmas", R"theme(
name = @i18n/themes/presets/christmas
author = Aurore
icon = tree
particles = snow
version = 1
description = Pine, cranberry and gold; the focus twinkles like the tree's lights.

[light]
background = #f4f6f1
surface = #ffffff
text = #12291e
accent = #b3162f
warning = #8a5200
danger = #c2410c
cadran/blue = #1f5f8b
cadran/violet = #5b3fa8
cadran/pink = #b0244a
brls/highlight/color1 = #1d6b45
brls/highlight/color2 = #a6780f
cadran/hl1 = #1d6b45
cadran/hl2 = #a6780f
cadran/bar = #cfe0d5
cadran/bar_today = #7fae93
cadran/glow = rgba(29, 107, 69, 0.12)
cadran/glow_strong = rgba(179, 22, 47, 0.16)

[dark]
background = #0d1f17
surface = #142b20
text = #f6f4ec
accent = #e8c15a
warning = #ff9f45
danger = #ff6b7a
cadran/blue = #9fd8ff
cadran/violet = #c9b0ff
cadran/pink = #ff8da1
brls/highlight/color1 = #e8c15a
brls/highlight/color2 = #ff7b8e
cadran/hl1 = #e8c15a
cadran/hl2 = #ff7b8e
brls/highlight/shadow = #e8c15a
cadran/bar = #22463a
cadran/bar_today = #3e7a5f
cadran/glow = rgba(232, 193, 90, 0.12)
cadran/glow_strong = rgba(224, 56, 79, 0.20)
cadran/brand/track = #1d3a2d
cadran/brand/fill = #e0384f
cadran/brand/glow = #e8c15a
brls/backdrop = rgba(4, 12, 8, 0.9)

[colors]
cadran/avatar0 = #e0384f
cadran/avatar1 = #e8c15a
cadran/avatar2 = #5cbf85
cadran/avatar3 = #9fd8ff
)theme" },

        { "new-year", R"theme(
name = @i18n/themes/presets/new_year
author = Aurore
icon = firework
particles = confetti
version = 1
description = Midnight and champagne: gold and silver sparkling in the focus.

[light]
background = #f3f2ef
surface = #ffffff
text = #12142a
accent = #c9a227
warning = #8a5200
danger = #e5484d
cadran/blue = #2a4fa8
cadran/violet = #5b3fa8
cadran/pink = #a63d70
brls/highlight/color1 = #8a6a0a
brls/highlight/color2 = #2a3a8f
cadran/hl1 = #8a6a0a
cadran/hl2 = #2a3a8f
cadran/bar = #e3ddca
cadran/bar_today = #c9a227
cadran/glow = rgba(201, 162, 39, 0.16)
cadran/glow_strong = rgba(201, 162, 39, 0.26)

[dark]
background = #0a0c18
surface = #141829
text = #f5f2e8
accent = #e9c46a
warning = #ffa94d
danger = #ff6b7a
cadran/blue = #8ab4ff
cadran/violet = #c3a6ff
cadran/pink = #ff9ec7
brls/highlight/color1 = #e9c46a
brls/highlight/color2 = #eef1f8
cadran/hl1 = #e9c46a
cadran/hl2 = #eef1f8
brls/highlight/shadow = #e9c46a
cadran/bar = #1f2540
cadran/bar_today = #3a4470
cadran/glow = rgba(233, 196, 106, 0.10)
cadran/glow_strong = rgba(192, 199, 214, 0.16)
cadran/brand/track = #1c2136
cadran/brand/fill = #e9c46a
cadran/brand/glow = #eef1f8
brls/backdrop = rgba(3, 4, 10, 0.9)

[colors]
cadran/avatar0 = #e9c46a
cadran/avatar1 = #c0c7d6
cadran/avatar2 = #8ab4ff
cadran/avatar3 = #ff9ec7
)theme" },

        // The season of the day (builtinThemeText): its text inherits it
        { "seasonal", nullptr },
    };
    return themes;
}

// ---------------------------------------------------------------------------
// builtin:seasonal: the preset of the day, by local date
// ---------------------------------------------------------------------------

namespace
{
// A range of days, both ends included, each year; it may run over the new
// year (from December to January). A month of 0 counts from the Lunar New
// Year instead: the days are offsets from it.
struct Season
{
    const char* id;
    int fromMonth, fromDay;
    int toMonth, toDay;
};

constexpr int LUNAR = 0;

// The first range the date falls in wins: the feasts first, then the
// seasons (the northern hemisphere's) around them, which cover every day.
constexpr Season SEASONS[] = {
    { "new-year", 12, 29, 1, 3 },
    { "christmas", 12, 1, 12, 28 },
    { "valentine", 2, 10, 2, 15 },
    { "lunar-new-year", LUNAR, -1, LUNAR, 14 }, // its eve to the Lantern Festival
    { "halloween", 10, 17, 11, 2 },
    { "spring", 3, 20, 6, 20 },
    { "summer", 6, 21, 9, 22 },
    { "autumn", 9, 23, 11, 30 },
    { "winter", 12, 1, 3, 19 },
};

// The Lunar New Year's day (Gregorian month and day), from 2025; a year
// out of the table has none
constexpr int LUNAR_FIRST_YEAR      = 2025;
constexpr int LUNAR_NEW_YEAR[][2] = {
    { 1, 29 }, { 2, 17 }, { 2, 6 }, { 1, 26 }, { 2, 13 }, { 2, 3 }, // 2025 to 2030
    { 1, 23 }, { 2, 11 }, { 1, 31 }, { 2, 19 }, { 2, 8 }, { 1, 28 }, // 2031 to 2036
};

// Days since 1970-01-01 of a Gregorian date (month 1 to 12)
int daysFromCivil(int year, int month, int day)
{
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const int yoe = year - era * 400;
    const int doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

bool inSeason(const Season& season, int year, int month, int day)
{
    if (season.fromMonth == LUNAR)
    {
        int index = year - LUNAR_FIRST_YEAR;
        if (index < 0 || index >= (int)std::size(LUNAR_NEW_YEAR))
            return false;
        int days  = daysFromCivil(year, month, day);
        int feast = daysFromCivil(year, LUNAR_NEW_YEAR[index][0], LUNAR_NEW_YEAR[index][1]);
        return days >= feast + season.fromDay && days <= feast + season.toDay;
    }
    int date = month * 100 + day;
    int from = season.fromMonth * 100 + season.fromDay;
    int to   = season.toMonth * 100 + season.toDay;
    return from <= to ? date >= from && date <= to : date >= from || date <= to;
}
}

const char* seasonalThemeId(std::time_t now)
{
    const std::tm* date = std::localtime(&now);
    if (date)
        for (const Season& season : SEASONS)
            if (inSeason(season, date->tm_year + 1900, date->tm_mon + 1, date->tm_mday))
                return season.id;
    return "winter";
}

std::string builtinThemeText(const BuiltinTheme& theme, std::time_t now)
{
    if (theme.text)
        return theme.text;
    return std::string("name = @i18n/themes/presets/seasonal\n"
                       "author = Aurore\n"
                       "version = 1\n"
                       "description = The season's preset, changing with the date.\n"
                       "inherit = builtin:")
        + seasonalThemeId(now) + "\n";
}

} // namespace brls

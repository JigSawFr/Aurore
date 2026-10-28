// cadran_palette — the boards' two palettes as plain values (no view, no
// nanovg), every colour Cadran draws with: cadran::install_theme puts them
// in borealis' themes under their keys (KEYS below), where a theme file
// can change any of them (docs/themes.md), and a host suite checks the
// contrast of every ink on every surface it is drawn on (tests/host;
// Theme::checkContrast does the same at run time, with CONTRAST below).
// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#pragma once

#include <cstdint>

namespace cadran::palette
{
/// A colour, 0 to 255 per channel.
struct Rgba
{
    uint8_t r, g, b, a;
};

/// Dark: the boards' navy. Light: the style board's cool white, the inks
/// darkened to 4.5:1 on every surface they are drawn on, the rail's active
/// pill included (teal `#0A705F`, amber `#8A5200`, coral `#B71C1C`); the quiet
/// text too, on both (text3 4.5:1 on the second surface). The focus ring
/// in the dark teal, its glow no lighter than the fill teal: 3:1 or more
/// on the background at any point of its pulse (WCAG 1.4.11). The dark
/// theme's hairline at 14 % white: the cards stay apart on a TV that
/// crushes its blacks.
struct Palette
{
    Rgba bg, text, text2, text3, surf, surf2, line, teal, teal_fill, teal_soft, amber, amber_fill, coral, coral_fill,
        coral_line, track, glow, glow_strong, amber_surf, amber_line, pill_ok, pill_ok_ink, rail_active, key_bg,
        key_fg, hl1, hl2, border;
    Rgba blue, violet; ///< a release note's alerts, a build's tags
    /// A removal's soft fill (the header's pill in danger).
    Rgba coral_soft;
    /// The empty part of a progress bar, behind a game's missing cover.
    Rgba well;
    /// The action tiles' fourth ink (cadran::List::tiles()), after teal, blue
    /// and violet: the third avatar's pink, darker on white.
    Rgba pink;
    /// The accounts' avatars, in turn: fills under a navy initial, alike on
    /// both themes.
    Rgba avatar0, avatar1, avatar2, avatar3;
    /// A switch's knob, white on either track, and its edge while off.
    Rgba knob, knob_line;
    /// A chart's bars, today's lighter.
    Rgba bar, bar_today;
    /// Behind a dialog or a picker: the navy, dimmed enough that the screen
    /// under it does not read through around a dialog's text and QR codes. A
    /// toast: a card of the second surface on the dark theme, navy on the
    /// light one, and its text.
    Rgba backdrop, toast, toast_text;
    /// The label on a teal or coral fill (a confirmation's, a primary
    /// button's): the navy.
    Rgba on_fill;
    /// A held button filling up (ui::hold_confirm), over its own colour.
    Rgba hold_fill;
    /// The rail's shadow over the tab while it is out (its darkest).
    Rgba rail_shadow;
    /// A press's flash: the fill teal, faint.
    Rgba click_pulse;
    /// What the app cannot do now (a read-only mode, ui::show_writable): the
    /// text, faded.
    Rgba text_off;
    /// The loading screen's bar (LoadingBar::setBrand), alike in both themes:
    /// its track, its fill and the fill's lighter end.
    Rgba brand_track, brand_fill, brand_glow;
};

constexpr Rgba NAVY = { 0x0B, 0x14, 0x24, 255 };

constexpr Palette DARK = {
    NAVY, // bg
    { 0xEE, 0xF2, 0xF8, 255 }, // text
    { 0xA9, 0xB5, 0xC9, 255 }, // text2
    { 0x83, 0x92, 0xAA, 255 }, // text3
    { 0x12, 0x1E, 0x33, 255 }, // surf
    { 0x1A, 0x2A, 0x45, 255 }, // surf2
    { 255, 255, 255, 36 }, // line
    { 0x2E, 0xC4, 0xA6, 255 }, // teal
    { 0x2E, 0xC4, 0xA6, 255 }, // teal_fill
    { 0x2E, 0xC4, 0xA6, 26 }, // teal_soft
    { 0xFF, 0xB5, 0x47, 255 }, // amber
    { 0xFF, 0xB5, 0x47, 255 }, // amber_fill
    { 0xFF, 0x7A, 0x7A, 255 }, // coral
    { 0xFF, 0x7A, 0x7A, 255 }, // coral_fill
    { 0xFF, 0x7A, 0x7A, 140 }, // coral_line
    { 255, 255, 255, 23 }, // track
    { 0x2E, 0xC4, 0xA6, 31 }, // glow
    { 0x2E, 0xC4, 0xA6, 51 }, // glow_strong
    { 0xFF, 0xB5, 0x47, 26 }, // amber_surf
    { 0xFF, 0xB5, 0x47, 90 }, // amber_line
    { 0x2E, 0xC4, 0xA6, 41 }, // pill_ok
    { 0x2E, 0xC4, 0xA6, 255 }, // pill_ok_ink
    { 0x2E, 0xC4, 0xA6, 36 }, // rail_active
    { 0xEE, 0xF2, 0xF8, 255 }, // key_bg
    NAVY, // key_fg
    { 0x2E, 0xC4, 0xA6, 255 }, // hl1
    { 0x9A, 0xF0, 0xDC, 255 }, // hl2
    { 255, 255, 255, 60 }, // border
    { 0x4F, 0xA3, 0xFF, 255 }, // blue
    { 0x9B, 0x8C, 0xF5, 255 }, // violet
    { 0xFF, 0x7A, 0x7A, 36 }, // coral_soft
    { 255, 255, 255, 46 }, // well
    { 0xE9, 0x6A, 0xA8, 255 }, // pink
    { 0x7C, 0x6C, 0xF2, 255 }, // avatar0
    { 0xFF, 0xB5, 0x47, 255 }, // avatar1
    { 0x4F, 0xA3, 0xFF, 255 }, // avatar2
    { 0xE9, 0x6A, 0xA8, 255 }, // avatar3
    { 0xFF, 0xFF, 0xFF, 255 }, // knob
    { 0x0B, 0x14, 0x24, 40 }, // knob_line
    { 0x5A, 0x71, 0x96, 255 }, // bar
    { 0x9F, 0xB4, 0xD4, 255 }, // bar_today
    { 3, 8, 18, 225 }, // backdrop
    { 0x1A, 0x2A, 0x45, 250 }, // toast
    { 0xEE, 0xF2, 0xF8, 255 }, // toast_text
    NAVY, // on_fill
    { 255, 255, 255, 70 }, // hold_fill
    { 0, 0, 0, 70 }, // rail_shadow
    { 0x2E, 0xC4, 0xA6, 38 }, // click_pulse
    { 0xEE, 0xF2, 0xF8, 110 }, // text_off
    { 0x1F, 0x33, 0x55, 255 }, // brand_track
    { 0x2E, 0xC4, 0xA6, 255 }, // brand_fill
    { 0x9A, 0xF0, 0xDC, 255 } // brand_glow
};

constexpr Palette LIGHT = {
    { 0xED, 0xF0, 0xF4, 255 }, // bg
    NAVY, // text
    { 0x3F, 0x4C, 0x63, 255 }, // text2
    { 0x5F, 0x6E, 0x88, 255 }, // text3
    { 0xFF, 0xFF, 0xFF, 255 }, // surf
    { 0xF3, 0xF5, 0xF8, 255 }, // surf2
    { 0x0B, 0x14, 0x24, 26 }, // line
    { 0x0A, 0x70, 0x5F, 255 }, // teal
    { 0x14, 0xA3, 0x8A, 255 }, // teal_fill
    { 0x14, 0xA3, 0x8A, 26 }, // teal_soft
    { 0x8A, 0x52, 0x00, 255 }, // amber
    { 0xE0, 0x93, 0x1B, 255 }, // amber_fill
    { 0xB7, 0x1C, 0x1C, 255 }, // coral
    { 0xE5, 0x48, 0x4D, 255 }, // coral_fill
    { 0xE3, 0x9A, 0x9A, 255 }, // coral_line
    { 0x0B, 0x14, 0x24, 26 }, // track
    { 0x2E, 0xC4, 0xA6, 41 }, // glow
    { 0x2E, 0xC4, 0xA6, 56 }, // glow_strong
    { 0xFF, 0xF3, 0xDF, 255 }, // amber_surf
    { 0xF3, 0xD5, 0x9C, 255 }, // amber_line
    { 0xD8, 0xF3, 0xEC, 255 }, // pill_ok
    { 0x0A, 0x6E, 0x5C, 255 }, // pill_ok_ink
    { 0x14, 0xA3, 0x8A, 36 }, // rail_active
    NAVY, // key_bg
    { 0xFF, 0xFF, 0xFF, 255 }, // key_fg
    { 0x0A, 0x70, 0x5F, 255 }, // hl1
    { 0x0F, 0x8A, 0x74, 255 }, // hl2
    { 0x0B, 0x14, 0x24, 60 }, // border
    { 0x1F, 0x5F, 0xC4, 255 }, // blue
    { 0x5B, 0x47, 0xC9, 255 }, // violet
    { 0xE5, 0x48, 0x4D, 28 }, // coral_soft
    { 0, 0, 0, 34 }, // well
    { 0xC2, 0x45, 0x7F, 255 }, // pink
    { 0x7C, 0x6C, 0xF2, 255 }, // avatar0
    { 0xFF, 0xB5, 0x47, 255 }, // avatar1
    { 0x4F, 0xA3, 0xFF, 255 }, // avatar2
    { 0xE9, 0x6A, 0xA8, 255 }, // avatar3
    { 0xFF, 0xFF, 0xFF, 255 }, // knob
    { 0x0B, 0x14, 0x24, 40 }, // knob_line
    { 0x7E, 0x90, 0xAE, 255 }, // bar
    { 0x4A, 0x5D, 0x7E, 255 }, // bar_today
    { 0x0B, 0x14, 0x24, 160 }, // backdrop
    { 0x0B, 0x14, 0x24, 240 }, // toast
    { 0xEE, 0xF2, 0xF8, 255 }, // toast_text
    NAVY, // on_fill
    { 255, 255, 255, 70 }, // hold_fill
    { 0, 0, 0, 70 }, // rail_shadow
    { 0x14, 0xA3, 0x8A, 38 }, // click_pulse
    { 0x0B, 0x14, 0x24, 110 }, // text_off
    { 0x1F, 0x33, 0x55, 255 }, // brand_track
    { 0x2E, 0xC4, 0xA6, 255 }, // brand_fill
    { 0x9A, 0xF0, 0xDC, 255 } // brand_glow
};

/// Each colour's theme keys: the Cadran one ("cadran/…"), and the core's it
/// also sets so that borealis' own views take the boards' look.
struct Key
{
    const char* name;
    Rgba Palette::*color;
};

constexpr Key KEYS[] = {
    { "brls/background", &Palette::bg },
    { "brls/clear", &Palette::bg },
    { "brls/text", &Palette::text },
    // A secondary text (a switch's Off, a placeholder) in the third ink,
    // what cannot be used now in the faded one
    { "brls/text_secondary", &Palette::text3 },
    { "brls/text_disabled", &Palette::text_off },
    { "brls/sidebar/background", &Palette::bg },
    { "brls/sidebar/separator", &Palette::line },
    { "brls/sidebar/active_item", &Palette::teal },
    { "brls/applet_frame/separator", &Palette::line },
    { "brls/header/border", &Palette::line },
    { "brls/highlight/background", &Palette::surf },
    { "brls/highlight/color1", &Palette::hl1 },
    { "brls/highlight/color2", &Palette::hl2 },
    { "brls/click_pulse", &Palette::click_pulse },
    { "brls/button/primary_enabled_background", &Palette::teal_fill },
    { "brls/button/primary_enabled_text", &Palette::on_fill },
    { "brls/button/default_enabled_background", &Palette::surf2 },
    { "brls/button/default_enabled_text", &Palette::text },
    { "brls/button/enabled_border_color", &Palette::border },
    { "brls/button/highlight_enabled_text", &Palette::teal },
    // A list line's plain value in the second ink, so the list screens
    // (cadran::List) take it as it is.
    { "brls/list/listItem_value_color", &Palette::text2 },
    { "brls/backdrop", &Palette::backdrop },
    { "brls/notification/background", &Palette::toast },
    { "brls/notification/text", &Palette::toast_text },
    { "cadran/navy", &Palette::on_fill },
    { "cadran/text", &Palette::text },
    { "cadran/text2", &Palette::text2 },
    { "cadran/text3", &Palette::text3 },
    { "cadran/text_off", &Palette::text_off },
    { "cadran/teal", &Palette::teal },
    { "cadran/teal_fill", &Palette::teal_fill },
    { "cadran/teal_soft", &Palette::teal_soft },
    { "cadran/amber", &Palette::amber },
    { "cadran/amber_fill", &Palette::amber_fill },
    { "cadran/coral", &Palette::coral },
    { "cadran/coral_fill", &Palette::coral_fill },
    { "cadran/coral_line", &Palette::coral_line },
    { "cadran/coral_soft", &Palette::coral_soft },
    { "cadran/track", &Palette::track },
    { "cadran/surf", &Palette::surf },
    { "cadran/surf2", &Palette::surf2 },
    { "cadran/line", &Palette::line },
    { "cadran/border", &Palette::border },
    { "cadran/glow", &Palette::glow },
    { "cadran/glow_strong", &Palette::glow_strong },
    { "cadran/amber_surf", &Palette::amber_surf },
    { "cadran/amber_line", &Palette::amber_line },
    { "cadran/pill_ok", &Palette::pill_ok },
    { "cadran/pill_ok_ink", &Palette::pill_ok_ink },
    { "cadran/rail_active", &Palette::rail_active },
    { "cadran/key_bg", &Palette::key_bg },
    { "cadran/key_fg", &Palette::key_fg },
    { "cadran/hl1", &Palette::hl1 },
    { "cadran/hl2", &Palette::hl2 },
    { "cadran/well", &Palette::well },
    { "cadran/blue", &Palette::blue },
    { "cadran/violet", &Palette::violet },
    { "cadran/pink", &Palette::pink },
    { "cadran/avatar0", &Palette::avatar0 },
    { "cadran/avatar1", &Palette::avatar1 },
    { "cadran/avatar2", &Palette::avatar2 },
    { "cadran/avatar3", &Palette::avatar3 },
    { "cadran/knob", &Palette::knob },
    { "cadran/knob_line", &Palette::knob_line },
    { "cadran/bar", &Palette::bar },
    { "cadran/bar_today", &Palette::bar_today },
    { "cadran/hold_fill", &Palette::hold_fill },
    { "cadran/rail_shadow", &Palette::rail_shadow },
    { "cadran/brand/track", &Palette::brand_track },
    { "cadran/brand/fill", &Palette::brand_fill },
    { "cadran/brand/glow", &Palette::brand_glow },
};

/// The pairs the palettes promise to keep readable (an ink on a surface, at
/// least `minimum`:1; a translucent one blended over the background first):
/// what the host suite checks of the palettes (tests/host/contrast_tests.cpp),
/// and Theme::checkContrast of a theme file changing them.
struct Contrast
{
    const char* ink;
    const char* surface;
    float minimum;
};

constexpr Contrast CONTRAST[] = {
    // The text and the inks, on the background, a card and the second surface.
    { "cadran/text", "brls/background", 4.5f },
    { "cadran/text", "cadran/surf", 4.5f },
    { "cadran/text", "cadran/surf2", 4.5f },
    { "cadran/text2", "brls/background", 4.5f },
    { "cadran/text2", "cadran/surf", 4.5f },
    { "cadran/text2", "cadran/surf2", 4.5f },
    { "cadran/text3", "brls/background", 4.5f },
    { "cadran/text3", "cadran/surf", 4.5f },
    { "cadran/text3", "cadran/surf2", 4.5f },
    { "cadran/teal", "brls/background", 4.5f },
    { "cadran/teal", "cadran/surf", 4.5f },
    { "cadran/teal", "cadran/surf2", 4.5f },
    { "cadran/amber", "brls/background", 4.5f },
    { "cadran/amber", "cadran/surf", 4.5f },
    { "cadran/amber", "cadran/surf2", 4.5f },
    { "cadran/coral", "brls/background", 4.5f },
    { "cadran/coral", "cadran/surf", 4.5f },
    { "cadran/coral", "cadran/surf2", 4.5f },
    { "cadran/blue", "brls/background", 4.5f },
    { "cadran/blue", "cadran/surf", 4.5f },
    { "cadran/blue", "cadran/surf2", 4.5f },
    { "cadran/violet", "brls/background", 4.5f },
    { "cadran/violet", "cadran/surf", 4.5f },
    { "cadran/violet", "cadran/surf2", 4.5f },
    // Where the palettes put an ink on a fill of their own.
    { "cadran/amber", "cadran/amber_surf", 4.5f },
    { "cadran/pill_ok_ink", "cadran/pill_ok", 4.5f },
    { "cadran/teal", "cadran/rail_active", 4.5f },
    { "cadran/key_fg", "cadran/key_bg", 4.5f },
    { "cadran/navy", "cadran/teal_fill", 4.5f },
    { "cadran/navy", "cadran/coral_fill", 4.5f },
    // The focus ring, both ends of its pulse, on the screen and on a card.
    { "brls/highlight/color1", "brls/background", 3.0f },
    { "brls/highlight/color2", "brls/background", 3.0f },
    { "brls/highlight/color1", "cadran/surf", 3.0f },
    { "brls/highlight/color2", "cadran/surf", 3.0f },
    // Secondary text in borealis' cells, on a card too.
    { "brls/text_secondary", "cadran/surf", 4.5f },
    { "brls/list/listItem_value_color", "cadran/surf", 4.5f },
    // Shapes that carry meaning, at 3:1 (WCAG 1.4.11): a tile's icon, a
    // chart's bars. A switch off is told by its knob and outline, in text3.
    { "cadran/pink", "cadran/surf", 3.0f },
    { "cadran/bar", "cadran/surf", 3.0f },
    { "cadran/bar_today", "cadran/surf", 3.0f },
};
} // namespace cadran::palette

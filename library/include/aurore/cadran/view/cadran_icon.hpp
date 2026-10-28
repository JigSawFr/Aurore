// CadranIcon — Cadran's icons, drawn as nanovg paths from
// the design boards' 24-unit SVG strokes (no SVG loader in borealis):
// today, timer, activity, shield, lock, globe, settings, tools, info, plus,
// stop, eye, check, moon, pause, list, lines, download, chevron-down, chevron-left,
// chevron-right, caret-up, caret-down, and "mark", the app's logo
// (ui::hooks' mark, else Aurore's); for the other screens, in the same strokes: key,
// unlock, phone, trash, server, pin, refresh, history, restore, save, upload,
// language, contrast, home, bolt, sparkle, bell, chip, code, heart, folder,
// file, flag, gamepad, calendar, alert, pencil, pencil-off, tag, user, sun, tv,
// text-normal, text-large, text-largest; for the palettes (theme files' `icon`): snowflake,
// lantern, blossom, wave, leaf, pumpkin, tree, firework, gem, dune, palette.
// A theme replaces "mark" with its `logo`, and any other icon with its
// `icon.<name>` picture, drawn as is or as a mask in the icon's colour
// (docs/themes.md, "Logos and images").
// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <aurore.hpp>
#include <aurore/core/theme_image.hpp>

/// One of Cadran's vector icons (`<CadranIcon icon="today">` in XML), drawn as
/// nanovg paths, or as the theme's picture for it (`icon.<name>`).
class CadranIcon : public aurore::View
{
  public:
    CadranIcon();

    void setIcon(const std::string& name);
    void setColor(NVGcolor color);
    /// "color" (setThemeColor, XML color="@theme/…")
    bool getStyleProperty(const std::string& name, aurore::StyleValue* value) override;
    bool setStyleProperty(const std::string& name, const aurore::StyleValue& value) override;
    /// The icon pops into place (grows from nothing, a little past its size,
    /// then settles): a check that was just earned. Nothing with the motion
    /// off (ui/effects).
    void pop();

    void draw(NVGcontext* vg, float x, float y, float width, float height,
        aurore::Style style, aurore::FrameContext* ctx) override;

    static aurore::View* create();

    /// Draws the icon `name` in `color`, filling a `width` by `height` box at
    /// the origin (nvgTranslate first): what a view of its own draws in an
    /// icon's place. False when no icon has that name (nothing drawn).
    /// The theme's picture for it when it has one (fitted in the box, its
    /// ratio kept), once loaded; else the drawing.
    static bool glyph(NVGcontext* vg, const std::string& name, NVGcolor color, float width, float height);

    /// Every icon's name but "mark" (what a theme's `icon.<name>` may replace).
    static const std::vector<std::string>& names();

  private:
    std::string icon = "today";
    NVGcolor color;
    aurore::Time pop_since = 0; ///< pop(): when it began (µs), 0 at rest
};

/// A few colours shown in an icon's place, the size of a row's icon square
/// (cadran::List::row): a palette's in its picker (ui::pick()'s `swatches`).
/// A rounded square of `background`, `ring` as a border inside it, `motif`
/// (a CadranIcon's name; "sparkle" when it has none or an unknown one) in
/// `ink`; around it a hairline in the theme's cadran/line, so that a dark
/// swatch still shows on a dark sheet.
/// With a `picture` (a theme's preview), that picture instead, cover-fitted
/// in the rounded square with the hairline: decoded on the decode thread
/// when the swatch is made (ThemeImages), the colours shown until it is
/// ready, or when it cannot be read; freed with the swatch.
class CadranSwatch : public aurore::View
{
  public:
    struct Colors
    {
        NVGcolor background;
        NVGcolor ring;
        NVGcolor ink;
        std::string motif;
        std::string picture; ///< "": the colours
    };

    explicit CadranSwatch(Colors colors);

    void draw(NVGcontext* vg, float x, float y, float width, float height,
        aurore::Style style, aurore::FrameContext* ctx) override;

  private:
    Colors colors;
    std::shared_ptr<aurore::ThemeImage> preview;
};

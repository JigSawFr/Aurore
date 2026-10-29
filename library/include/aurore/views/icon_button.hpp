// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/views/button.hpp>
#include <aurore/views/image.hpp>

namespace aurore
{

// A Button with an icon before its text, or an icon alone (a round button)
// when it has no text. The icon is a picture, a Material Icons glyph (drawn
// in the text's colour, so it follows the style and the state), or a view
// of the app's. Every Button attribute and style applies.
//
// XML: icon (a picture: "@res/…" or a file path), glyph (a Material Icons
// glyph, e.g. "&#xE8B8;"), iconSize, and the Button attributes.
class IconButton : public Button
{
  public:
    IconButton();

    void setIcon(const std::string& path);
    void setGlyph(const std::string& glyph);
    void setIconView(View* view);
    void setIconSize(float size);

    void setText(const std::string& text);

    bool applyXMLAttribute(std::string name, std::string value) override;
    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;

    static View* create();

  private:
    Label* text    = nullptr; // the Button's label
    View* icon     = nullptr;
    Label* glyph   = nullptr; // the icon, when it is a glyph
    float iconSize = 24;

    void replaceIcon(View* view);
    void updateShape();
};

} // namespace aurore

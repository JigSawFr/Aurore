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

#include <aurore/views/button.hpp>
#include <aurore/views/image.hpp>

namespace brls
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

} // namespace brls

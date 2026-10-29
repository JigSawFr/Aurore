// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#include <aurore/core/application.hpp>
#include <aurore/views/icon_button.hpp>

namespace aurore
{

IconButton::IconButton()
{
    this->text = (Label*)this->getView("aurore/button/label");

    this->registerFilePathXMLAttribute("icon", [this](const std::string& value)
        { this->setIcon(value); });
    this->registerStringXMLAttribute("glyph", [this](const std::string& value)
        { this->setGlyph(value); });
    this->registerFloatXMLAttribute("iconSize", [this](float value)
        { this->setIconSize(value); });
}

void IconButton::replaceIcon(View* view)
{
    if (this->icon)
        this->removeView(this->icon);
    this->icon  = view;
    this->glyph = nullptr;

    if (view)
    {
        view->setDimensions(this->iconSize, this->iconSize);
        view->setShrink(0);
        this->addView(view, 0);
    }
    this->updateShape();
}

void IconButton::setIcon(const std::string& path)
{
    auto* image = new Image();
    image->setScalingType(ImageScalingType::FIT);
    image->setImageFromFile(path);
    this->replaceIcon(image);
}

void IconButton::setGlyph(const std::string& text)
{
    auto* label = new Label();
    label->setFont(Application::getFont(FONT_MATERIAL_ICONS));
    label->setFontSize(this->iconSize);
    label->setSingleLine(true);
    label->setHorizontalAlign(HorizontalAlign::CENTER);
    label->setVerticalAlign(VerticalAlign::CENTER);
    label->setText(text);
    this->replaceIcon(label);
    this->glyph = label;
}

void IconButton::setIconView(View* view)
{
    this->replaceIcon(view);
}

void IconButton::setIconSize(float size)
{
    this->iconSize = size;
    if (this->icon)
        this->icon->setDimensions(size, size);
    if (this->glyph)
        this->glyph->setFontSize(size);
}

void IconButton::setText(const std::string& text)
{
    Button::setText(text);
    this->updateShape();
}

// With no text: the icon alone, in a round button
void IconButton::updateShape()
{
    bool iconOnly  = this->icon && this->text->getFullText().empty();
    Visibility vis = iconOnly ? Visibility::GONE : Visibility::VISIBLE;

    if (this->text->getVisibility() != vis)
        this->text->setVisibility(vis);

    if (this->icon)
    {
        float gap = iconOnly ? 0 : 12;
        if (this->icon->getMarginRight() != gap)
            this->icon->setMarginRight(gap);
    }

    if (iconOnly)
    {
        float side = Application::getStyle()["aurore/button/padding_top_bottom"];
        if (this->getPaddingLeft() != side)
            this->setPadding(side, side, side, side);
        float radius = this->iconSize / 2 + side;
        if (this->getCornerRadius() != radius)
        {
            this->setCornerRadius(radius);
            this->setHighlightCornerRadius(radius);
        }
    }
}

// "text" is forwarded to the label: the shape follows it
bool IconButton::applyXMLAttribute(std::string name, std::string value)
{
    bool applied = Button::applyXMLAttribute(name, value);
    if (name == "text")
        this->updateShape();
    return applied;
}

void IconButton::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    // The glyph in the text's colour, whatever the style and the state set it to
    if (this->glyph)
        this->glyph->setTextColor(this->text->getTextColor());
    Button::draw(vg, x, y, width, height, style, ctx);
}

View* IconButton::create()
{
    return new IconButton();
}

} // namespace aurore

// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#include "aurore/views/cells/cell_radio.hpp"

namespace aurore
{

const std::string radioCellXML = R"xml(
    <aurore:Box
        width="auto"
        height="@style/aurore/sidebar/item_height"
        focusable="true"
        paddingTop="12.5"
        paddingBottom="12.5"
        alignItems="center">

        <aurore:Label
            id="aurore/rediocell/title"
            width="auto"
            height="auto"
            grow="1"
            fontSize="@style/aurore/sidebar/item_font_size"
            marginLeft="@style/aurore/listitem/descriptionIndent"
            marginRight="@style/aurore/listitem/descriptionIndent" />

        <aurore:CheckBox
            id="aurore/rediocell/checkbox"
            visibility="gone"
            marginRight="@style/aurore/listitem/descriptionIndent"/>

    </aurore:Box>
)xml";

CheckBox::CheckBox()
{
    float size = Application::getStyle()["aurore/listitem/selectRadius"] * 2;
    setWidth(size);
    setHeight(size);
}

void CheckBox::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    float radius  = style["aurore/listitem/selectRadius"];
    float centerX = x + width / 2;
    float centerY = y + height / 2;

    int thickness = roundf(radius * 0.10f);

    // Background
    nvgFillColor(vg, a(ctx->theme["aurore/list/listItem_value_color"]));
    nvgBeginPath(vg);
    nvgCircle(vg, centerX, centerY, radius);
    nvgFill(vg);

    // Check mark
    nvgFillColor(vg, a(ctx->theme["aurore/background"]));

    // Long stroke
    nvgSave(vg);
    nvgTranslate(vg, centerX, centerY);
    nvgRotate(vg, -NVG_PI / 4.0f);

    nvgBeginPath(vg);
    nvgRect(vg, -(radius * 0.55f), 0, radius * 1.3f, thickness);
    nvgFill(vg);
    nvgRestore(vg);

    // Short stroke
    nvgSave(vg);
    nvgTranslate(vg, centerX - (radius * 0.65f), centerY);
    nvgRotate(vg, NVG_PI / 4.0f);

    nvgBeginPath(vg);
    nvgRect(vg, 0, -(thickness / 2), radius * 0.53f, thickness);
    nvgFill(vg);

    nvgRestore(vg);
}

View* CheckBox::create()
{
    return new CheckBox();
}

RadioCell::RadioCell()
{
    this->inflateFromXMLString(radioCellXML);
    this->growWithText();

    this->registerStringXMLAttribute("title", [this](std::string value)
        { this->title->setText(value); });
}

void RadioCell::setSelected(bool selected)
{
    bool changed   = this->selected != selected;
    this->selected = selected;
    if (changed && selected)
        this->announceAccessibilityValue();
    this->checkbox->setVisibility(selected ? Visibility::VISIBLE : Visibility::GONE);
    this->title->setThemeColor("textColor", selected ? "aurore/list/listItem_value_color" : "aurore/text");
}

bool RadioCell::getSelected()
{
    return this->selected;
}

View* RadioCell::create()
{
    return new RadioCell();
}

} // namespace aurore

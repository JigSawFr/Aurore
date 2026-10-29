// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#include "aurore/views/cells/cell_detail.hpp"

namespace aurore
{

const std::string detailCellXML = R"xml(
    <aurore:Box
        width="auto"
        height="@style/aurore/sidebar/item_height"
        focusable="true"
        paddingTop="12.5"
        paddingBottom="12.5"
        paddingLeft="@style/aurore/listitem/descriptionIndent"
        paddingRight="@style/aurore/listitem/descriptionIndent"
        alignItems="center">

        <aurore:Label
            id="aurore/rediocell/title"
            width="auto"
            height="auto"
            grow="1"
            fontSize="@style/aurore/sidebar/item_font_size"
            marginRight="@style/aurore/listitem/descriptionIndent" />

        <aurore:Label
            id="aurore/rediocell/detail"
            horizontalAlign="right"
            maxWidth="300"
            height="auto"
            shrink="0"
            fontSize="@style/aurore/sidebar/item_font_size"
            textColor="@theme/aurore/list/listItem_value_color"/>

    </aurore:Box>
)xml";

DetailCell::DetailCell()
{
    this->inflateFromXMLString(detailCellXML);
    this->growWithText();

    this->registerStringXMLAttribute("title", [this](std::string value)
        { this->title->setText(value); });
}

void DetailCell::setText(std::string title)
{
    this->title->setText(title);
}

void DetailCell::setTextColor(NVGcolor color)
{
    this->title->setTextColor(color);
}

void DetailCell::setDetailText(std::string title)
{
    bool changed = this->detail->getFullText() != title;
    this->detail->setText(title);
    if (changed)
        this->announceAccessibilityValue();
}

void DetailCell::setDetailTextColor(NVGcolor color)
{
    this->detail->setTextColor(color);
}

View* DetailCell::create()
{
    return new DetailCell();
}

bool DetailCell::setBoundText(const std::string& text)
{
    this->setText(text);
    return true;
}

std::string DetailCell::getDefaultAccessibilityLabel()
{
    return this->title->getFullText();
}

std::string DetailCell::getDefaultAccessibilityValue()
{
    std::string value;
    for (View* child : this->getChildren())
    {
        if (child == this->title || child->isFocusable())
            continue;
        std::string text = child->getAccessibilityText();
        // The "›" of a line that opens something is not read out.
        size_t chevron;
        while ((chevron = text.find("\u203a")) != std::string::npos)
            text.erase(chevron, 3);
        while (!text.empty() && text.back() == ' ')
            text.pop_back();
        if (!text.empty())
            value += (value.empty() ? "" : ", ") + text;
    }
    return value;
}

} // namespace aurore

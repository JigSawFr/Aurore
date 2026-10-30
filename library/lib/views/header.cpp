// SPDX-FileCopyrightText: 2019-2021 natinusala
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-License-Identifier: Apache-2.0

#include <aurore/views/header.hpp>

namespace aurore
{

constexpr std::string_view headerXML = R"xml(
    <aurore:Box
        width="auto"
        height="auto"
        axis="row"
        compact:axis="column"
        compact:shrink="0"
        paddingTop="@style/aurore/header/padding_top_bottom"
        paddingBottom="@style/aurore/header/padding_top_bottom"
        lineBottom="1px"
        lineColor="@theme/aurore/header/border" >

        <aurore:Rectangle
            width="@style/aurore/header/rectangle_width"
            height="auto"
            color="@theme/aurore/header/rectangle"
            marginRight="@style/aurore/header/rectangle_margin"
            compact:visibility="gone" />

        <aurore:Label
            id="aurore/header/title"
            width="auto"
            height="auto"
            fontSize="@style/aurore/header/font_size" />

        <aurore:Padding compact:visibility="gone" />

        <aurore:Label
            id="aurore/header/subtitle"
            width="auto"
            height="auto"
            fontSize="@style/aurore/header/font_size"
            textColor="@theme/aurore/header/subtitle"
            visibility="gone"
            horizontalAlign="right"
            compact:horizontalAlign="left"
            compact:singleLine="true" />

    </aurore:Box>
)xml";

Header::Header()
{
    this->inflateFromXMLString(headerXML);

    this->registerStringXMLAttribute("title", [this](std::string value)
        { this->setTitle(value); });

    this->registerStringXMLAttribute("subtitle", [this](std::string value)
        { this->setSubtitle(value); });
}

void Header::setTitle(std::string title)
{
    this->title->setText(title);
}

void Header::setSubtitle(std::string subtitle)
{
    this->subtitle->setVisibility(Visibility::VISIBLE);
    this->subtitle->setText(subtitle);
}

View* Header::create()
{
    return new Header();
}

} // namespace aurore

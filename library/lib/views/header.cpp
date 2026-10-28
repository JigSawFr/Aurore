/*
    Copyright 2019-2021 natinusala
    Copyright 2019 p-sam

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

#include <aurore/views/header.hpp>

namespace aurore
{

const std::string headerXML = R"xml(
    <aurore:Box
        width="auto"
        height="auto"
        axis="row"
        paddingTop="@style/aurore/header/padding_top_bottom"
        paddingBottom="@style/aurore/header/padding_top_bottom"
        lineBottom="1px"
        lineColor="@theme/aurore/header/border" >

        <aurore:Rectangle
            width="@style/aurore/header/rectangle_width"
            height="auto"
            color="@theme/aurore/header/rectangle"
            marginRight="@style/aurore/header/rectangle_margin" />

        <aurore:Label
            id="aurore/header/title"
            width="auto"
            height="auto"
            fontSize="@style/aurore/header/font_size" />

        <aurore:Padding />

        <aurore:Label
            id="aurore/header/subtitle"
            width="auto"
            height="auto"
            fontSize="@style/aurore/header/font_size"
            textColor="@theme/aurore/header/subtitle"
            visibility="gone"
            horizontalAlign="right" />

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

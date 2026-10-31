// SPDX-FileCopyrightText: 2019-2021 natinusala
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-License-Identifier: Apache-2.0

#include <math.h>

#include <aurore/core/application.hpp>
#include <aurore/core/touch/tap_gesture.hpp>
#include <aurore/views/button.hpp>

namespace aurore
{

constexpr std::string_view buttonXML = R"xml(
    <aurore:Box
        width="auto"
        height="auto"
        axis="row"
        focusable="true"
        justifyContent="center"
        alignItems="center"
        paddingTop="@style/aurore/button/padding_top_bottom"
        paddingRight="@style/aurore/button/padding_sides"
        paddingBottom="@style/aurore/button/padding_top_bottom"
        paddingLeft="@style/aurore/button/padding_sides"
        cornerRadius="@style/aurore/button/corner_radius"
        highlightCornerRadius="@style/aurore/button/corner_radius">

    <aurore:Label
        id="aurore/button/label"
        width="auto"
        height="auto"
        fontSize="@style/aurore/button/text_size"
        horizontalAlign="center" />

    </aurore:Box>
)xml";

Button::Button()
{
    this->inflateFromXMLString(buttonXML);

    this->forwardXMLAttribute("text", this->label);
    this->forwardXMLAttribute("singleLine", this->label);
    this->forwardXMLAttribute("fontSize", this->label);
    this->forwardXMLAttribute("textColor", this->label);
    this->forwardXMLAttribute("lineHeight", this->label);
    this->forwardXMLAttribute("animated", this->label);
    this->forwardXMLAttribute("autoAnimate", this->label);
    this->forwardXMLAttribute("textHorizontalAlign", this->label, "horizontalAlign");

    this->registerColorXMLAttribute("textColor", [this](NVGcolor color)
        { this->setTextColor(color); });

    AURORE_REGISTER_ENUM_XML_ATTRIBUTE(
        "style", const ButtonStyle*, this->setStyle,
        {
            { "default", &BUTTONSTYLE_DEFAULT },
            { "primary", &BUTTONSTYLE_PRIMARY },
            { "highlight", &BUTTONSTYLE_HIGHLIGHT },
            { "bordered", &BUTTONSTYLE_BORDERED },
            { "borderless", &BUTTONSTYLE_BORDERLESS },
        });

    AURORE_REGISTER_ENUM_XML_ATTRIBUTE(
        "state", ButtonState, this->setState,
        {
            { "enabled", ButtonState::ENABLED },
            { "disabled", ButtonState::DISABLED },
        });

    this->applyStyle();

    this->addGestureRecognizer(new TapGestureRecognizer(this));
}

void Button::applyStyle()
{
    Style style = Application::getStyle();

    this->setShadowType(this->style->shadowType);
    this->setHideHighlightBackground(this->style->hideHighlightBackground);

    if (this->style->highlightPadding != "")
        this->setHighlightPadding(style[this->style->highlightPadding]);
    else
        this->setHighlightPadding(0.0f);

    if (this->style->borderThickness != "")
        this->setBorderThickness(style[this->style->borderThickness]);
    else
        this->setBorderThickness(0.0f);

    std::string backgroundColor;
    std::string textColor;
    std::string borderColor;

    switch (this->state)
    {
        case ButtonState::ENABLED:
            backgroundColor = this->style->enabledBackgroundColor;
            textColor       = this->style->enabledLabelColor;
            borderColor     = this->style->enabledBorderColor;

            break;
        case ButtonState::DISABLED:
            backgroundColor = this->style->disabledBackgroundColor;
            textColor       = this->style->disabledLabelColor;
            borderColor     = this->style->disabledBorderColor;

            break;
    }

    if (backgroundColor != "")
        this->setThemeColor("backgroundColor", backgroundColor);
    else
    {
        this->setThemeColor("backgroundColor", "");
        this->setBackground(ViewBackground::NONE);
    }

    if (textColorOverritten)
    {
        this->label->setThemeColor("textColor", "");
        this->label->setTextColor(this->textColor);
    }
    else
        this->label->setThemeColor("textColor", textColor);

    if (this->getBorderThickness() > 0.0f)
        this->setThemeColor("borderColor", borderColor);
    else
        this->setThemeColor("borderColor", "");
}

void Button::onFocusGained()
{
    Box::onFocusGained();

    this->setShadowVisibility(false);
}

void Button::onFocusLost()
{
    Box::onFocusLost();

    this->setShadowVisibility(true);
}

void Button::setStyle(const ButtonStyle* style)
{
    this->style         = style;
    textColorOverritten = false;
    this->applyStyle();
}

void Button::setState(ButtonState state)
{
    this->state = state;
    this->setStyleState(StyleState::DISABLED, state == ButtonState::DISABLED);
    this->applyStyle();
    this->setActionsAvailable(state == ButtonState::ENABLED);
}

ButtonState Button::getState()
{
    return this->state;
}

void Button::setTextColor(NVGcolor color)
{
    textColor           = color;
    textColorOverritten = true;
    applyStyle();
}

void Button::setText(std::string text)
{
    this->label->setText(text);
}

void Button::setFontSize(float value)
{
    this->label->setFontSize(value);
}

std::string Button::getText()
{
    return label->getFullText();
}

bool Button::getStyleProperty(const std::string& name, StyleValue* value)
{
    if (name != "textColor")
        return Box::getStyleProperty(name, value);
    *value = StyleValue::of(this->label->getTextColor());
    return true;
}

bool Button::setStyleProperty(const std::string& name, const StyleValue& value)
{
    if (name != "textColor")
        return Box::setStyleProperty(name, value);
    if (value.type != StyleValue::Type::COLOR)
        return false;
    if (!this->transitionStyle("textColor", value))
        this->label->setTextColor(value.color);
    return true;
}

bool Button::setBoundText(const std::string& text)
{
    this->setText(text);
    return true;
}

View* Button::create()
{
    return new Button();
}

} // namespace aurore

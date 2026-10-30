// SPDX-FileCopyrightText: 2019-2021 natinusala
// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#include <aurore/core/application.hpp>
#include <aurore/core/i18n.hpp>
#include <aurore/core/touch/tap_gesture.hpp>
#include <aurore/views/dialog.hpp>
#include <aurore/views/label.hpp>

using namespace aurore::literals;

// TODO: different open animation?

namespace aurore
{

constexpr std::string_view dialogXML = R"xml(
    <aurore:Box
        width="auto"
        height="auto"
        axis="column"
        justifyContent="center"
        alignItems="center"
        backgroundColor="@theme/aurore/backdrop">

        <aurore:AppletFrame
            id="aurore/dialog/applet"
            width="720"
            compact:width="94%"
            height="auto"
            headerHidden="true"
            footerHidden="true"
            cornerRadius="4"
            backgroundColor="@theme/aurore/background">

            <aurore:Box
                width="auto"
                height="auto"
                grow="1"
                axis="column">

                <aurore:Box
                    id="aurore/dialog/container"
                    width="auto"
                    height="auto"
                    grow="1"
                    axis="column"/>

                <aurore:Rectangle
                    id="aurore/dialog/button3/separator"
                    width="auto"
                    height="2"
                    color="@theme/aurore/sidebar/separator"
                    visibility="gone" />

                <aurore:Button
                    id="aurore/dialog/button3"
                    width="auto"
                    height="72"
                    axis="column"
                    focusable="true"
                    justifyContent="center"
                    alignItems="center"
                    highlightCornerRadius="6"
                    fontSize="@style/aurore/dialog/fontSize"
                    style="borderless"
                    textColor="@theme/aurore/accent"
                    text="Continue"
                    visibility="gone"/>

                <aurore:Box
                    width="auto"
                    height="72"
                    axis="row"
                    justifyContent="spaceEvenly"
                    alignItems="stretch"
                    lineTop="2px"
                    lineColor="@theme/aurore/sidebar/separator"
                    visibility="gone">

                    <aurore:Button
                        id="aurore/dialog/button1"
                        width="0"
                        height="auto"
                        grow="1"
                        focusable="true"
                        justifyContent="center"
                        alignItems="center"
                        highlightCornerRadius="6"
                        text="Continue"
                        style="borderless"
                        fontSize="@style/aurore/dialog/fontSize"
                        textColor="@theme/aurore/accent"
                        visibility="gone"/>

                    <aurore:Rectangle
                        id="aurore/dialog/button2/separator"
                        width="2"
                        height="auto"
                        color="@theme/aurore/sidebar/separator"
                        visibility="gone" />

                    <aurore:Button
                        id="aurore/dialog/button2"
                        width="0"
                        height="auto"
                        grow="1"
                        focusable="true"
                        justifyContent="center"
                        alignItems="center"
                        highlightCornerRadius="6"
                        text="Continue"
                        style="borderless"
                        fontSize="@style/aurore/dialog/fontSize"
                        textColor="@theme/aurore/accent"
                        visibility="gone"/>

                </aurore:Box>
            
            </aurore:Box>

        </aurore:AppletFrame>

    </aurore:Box>
)xml";

Dialog::Dialog(Box* contentView)
{
    this->inflateFromXMLString(dialogXML);
    appletFrame->setThemeArea(ThemeArea::DIALOG);
    container->addView(contentView);

    appletFrame->registerAction(
        "hints/back"_i18n, BUTTON_B, [this](View* view)
        {
            if (cancelable)
                this->close(this->closeCallback);
            return cancelable; },
        false, false, SOUND_BACK);

    // A tap on the backdrop, outside the dialog, cancels it as B does
    this->addGestureRecognizer(new TapGestureRecognizer([this](TapGestureStatus status, Sound* soundToPlay)
        {
            if (status.state != GestureState::END || !this->cancelable)
                return;
            if (this->appletFrame->getFrame().pointInside(status.position))
                return;
            *soundToPlay = SOUND_BACK;
            this->close(this->closeCallback); }));
}

Dialog::Dialog(std::string text)
{
    Style style = Application::getStyle();

    Label* label    = new Label();
    this->textLabel = label;
    label->setText(text);
    label->setFontSize(style["aurore/dialog/fontSize"]);
    label->setHorizontalAlign(HorizontalAlign::CENTER);
    label->setSingleLine(false);

    Box* box = new Box();
    box->addView(label);
    box->setAlignItems(AlignItems::CENTER);
    box->setJustifyContent(JustifyContent::CENTER);
    box->setPadding(style["aurore/dialog/paddingTopBottom"], style["aurore/dialog/paddingLeftRight"], style["aurore/dialog/paddingTopBottom"], style["aurore/dialog/paddingLeftRight"]);

    this->inflateFromXMLString(dialogXML);
    appletFrame->setThemeArea(ThemeArea::DIALOG);
    container->addView(box);

    appletFrame->registerAction(
        "hints/back"_i18n, BUTTON_B, [this](View* view)
        {
            if (cancelable)
                this->close(this->closeCallback);
            return cancelable; },
        false, false, SOUND_BACK);

    // A tap on the backdrop, outside the dialog, cancels it as B does
    this->addGestureRecognizer(new TapGestureRecognizer([this](TapGestureStatus status, Sound* soundToPlay)
        {
            if (status.state != GestureState::END || !this->cancelable)
                return;
            if (this->appletFrame->getFrame().pointInside(status.position))
                return;
            *soundToPlay = SOUND_BACK;
            this->close(this->closeCallback); }));
}

void Dialog::addButton(std::string label, VoidEvent::Callback cb)
{
    if (this->buttons.size() >= 3)
        return;

    DialogButton* button = new DialogButton();
    button->label        = label;
    button->cb           = cb;

    this->buttons.push_back(button);

    this->rebuildButtons();
}

void Dialog::open()
{
    Application::pushActivity(new Activity(this));
}

void Dialog::close(std::function<void(void)> cb)
{
    Box::dismiss(cb);
}

void Dialog::setCancelable(bool cancelable)
{
    this->cancelable = cancelable;
}

void Dialog::setCloseCallback(std::function<void(void)> cb)
{
    this->closeCallback = cb;
}

void Dialog::rebuildButtons()
{
    if (this->buttons.size() > 0)
    {
        setLastFocusedView(button1);
        button1->getParent()->setVisibility(Visibility::VISIBLE);

        button1->setVisibility(Visibility::VISIBLE);
        button1->setText(buttons[0]->label);
        button1->registerClickAction([this](View* view)
            {
            buttonClick(buttons[0]);
            return true; });
    }

    if (this->buttons.size() > 1)
    {
        button2separator->setVisibility(Visibility::VISIBLE);
        button2->setVisibility(Visibility::VISIBLE);
        button2->setText(buttons[1]->label);
        button2->registerClickAction([this](View* view)
            {
            buttonClick(buttons[1]);
            return true; });
    }

    if (this->buttons.size() > 2)
    {
        button3separator->setVisibility(Visibility::VISIBLE);
        button3->setVisibility(Visibility::VISIBLE);
        button3->setText(buttons[2]->label);
        button3->registerClickAction([this](View* view)
            {
            buttonClick(buttons[2]);
            return true; });
    }
}

void Dialog::buttonClick(DialogButton* button)
{
    if (!this->dismissOnButtonClick)
    {
        if (button->cb)
            button->cb();
        return;
    }

    dismiss([button]
        {
        if (button->cb)
            button->cb(); });
}

void Dialog::setText(const std::string& text)
{
    if (this->textLabel)
        this->textLabel->setText(text);
}

std::string Dialog::getText()
{
    return this->textLabel ? this->textLabel->getFullText() : "";
}

void Dialog::setButtonText(size_t index, const std::string& label)
{
    if (index >= this->buttons.size())
        return;

    this->buttons[index]->label = label;
    Button* buttons[]           = { this->button1, this->button2, this->button3 };
    buttons[index]->setText(label);
}

void Dialog::setDismissOnButtonClick(bool dismiss)
{
    this->dismissOnButtonClick = dismiss;
}

AppletFrame* Dialog::getAppletFrame()
{
    return appletFrame;
}

Dialog::~Dialog()
{
    for (auto& i : this->buttons)
    {
        delete i;
    }
}

} // namespace aurore

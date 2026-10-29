/*
    Copyright 2019-2021 natinusala
    Copyright 2021 XITRIX
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

#include <aurore/core/application.hpp>
#include <aurore/core/i18n.hpp>
#include <aurore/core/logger.hpp>
#include <aurore/core/util.hpp>
#include <aurore/views/applet_frame.hpp>
#include <aurore/views/dialog.hpp>
#include <aurore/views/hint.hpp>

using namespace aurore::literals;

namespace aurore
{

static const char* HEADER_ID = "aurore/applet_frame/header";
static const char* FOOTER_ID = "aurore/applet_frame/footer";

const std::string appletFrameXML = R"xml(
    <aurore:Box
        width="auto"
        height="auto"
        axis="column"
        justifyContent="spaceBetween">

        <!-- Header -->
        <aurore:Box
            id="aurore/applet_frame/header"
            width="auto"
            height="@style/aurore/applet_frame/header_height"
            axis="row"
            justifyContent="spaceBetween"
            paddingLeft="@style/aurore/applet_frame/header_padding_sides"
            paddingRight="@style/aurore/applet_frame/header_padding_sides"
            marginLeft="@style/aurore/applet_frame/padding_sides"
            marginRight="@style/aurore/applet_frame/padding_sides"
            lineColor="@theme/aurore/applet_frame/separator"
            lineBottom="1px">

            <aurore:Box
                width="auto"
                height="auto"
                axis="row"
                paddingTop="@style/aurore/applet_frame/header_padding_top_bottom"
                paddingBottom="@style/aurore/applet_frame/header_padding_top_bottom">
                <aurore:Image
                    id="aurore/applet_frame/title_icon"
                    width="auto"
                    height="auto"
                    marginRight="@style/aurore/applet_frame/header_image_title_spacing"
                    visibility="gone" />

                <aurore:Label
                    id="aurore/applet_frame/title_label"
                    width="auto"
                    height="auto"
                    marginTop="@style/aurore/applet_frame/header_title_top_offset"
                    fontSize="@style/aurore/applet_frame/header_title_font_size" />
            </aurore:Box>

            <aurore:Box
                id="aurore/applet_frame/hint_box"
                width="auto"
                height="auto"
                marginTop="@style/aurore/applet_frame/header_title_top_offset"
                axis="row"/>

        </aurore:Box>

        <!-- Content will be injected here with grow="1.0" -->

        <!--
            Footer
            Direction inverted so that the bottom left text can be
            set to visibility="gone" without affecting the hint
        -->
        <aurore:BottomBar
            id="aurore/applet_frame/footer"/>

    </aurore:Box>
)xml";

AppletFrame::AppletFrame()
{
    this->inflateFromXMLString(appletFrameXML);
    header->setThemeArea(ThemeArea::HEADER);

    // Edge to edge: the header and the footer grow into the safe area,
    // their backgrounds up to the window's edges
    header->setBoxSizing(BoxSizing::CONTENT_BOX);
    header->setSafeArea(SafeAreaEdges::TOP);
    footer->setBoxSizing(BoxSizing::CONTENT_BOX);
    footer->setSafeArea(SafeAreaEdges::BOTTOM);
    Box::setSafeArea(SafeAreaEdges::HORIZONTAL);

    this->forwardXMLAttribute("iconInterpolation", this->icon, "interpolation");

    AURORE_REGISTER_ENUM_XML_ATTRIBUTE(
        "style", HeaderStyle, this->setHeaderStyle,
        {
            { "regular", HeaderStyle::REGULAR },
            { "popup", HeaderStyle::POPUP },
        });

    this->registerBoolXMLAttribute("headerHidden", [this](bool value)
        { this->setHeaderVisibility(value ? Visibility::GONE : Visibility::VISIBLE); });

    this->registerBoolXMLAttribute("footerHidden", [this](bool value)
        {
        if(HIDE_BOTTOM_BAR)
            this->setFooterVisibility(Visibility::GONE);
        else
            this->setFooterVisibility(value ? Visibility::GONE : Visibility::VISIBLE); });

    this->registerAction(
        "hints/back"_i18n, BUTTON_B, [this](View* view)
        {
            // A frame given its content by setContentView alone has an
            // empty stack: dismiss the frame itself
            if (this->contentViewStack.empty())
                this->dismiss();
            else
                this->contentViewStack.back()->dismiss();
            return true; },
        false, false, SOUND_BACK);
}

AppletFrame::AppletFrame(View* contentView)
    : AppletFrame::AppletFrame()
{
    contentViewStack.push_back(contentView);
    setContentView(contentView);
}

void AppletFrame::setIcon(std::string path)
{
    if (path.empty())
    {
        this->icon->setVisibility(Visibility::GONE);
    }
    else
    {
        this->icon->setVisibility(Visibility::VISIBLE);
        this->icon->setImageFromFile(path);
    }
}

void AppletFrame::setHeaderVisibility(Visibility visibility)
{
    header->setVisibility(visibility);
    this->reapplySafeArea();
}

void AppletFrame::setFooterVisibility(Visibility visibility)
{
    footer->setVisibility(visibility);
    this->reapplySafeArea();
}

void AppletFrame::setSafeArea(SafeAreaEdges edges)
{
    this->safeAreaManaged = false;
    Box::setSafeArea(edges);
}

// The header or the footer: one of the frame's own children (not yet there
// while the frame is inflated, maybe taken out by an app), not one of a frame
// in the content
static bool isBar(View* child, const char* id)
{
    return child && child->getId() == id;
}

SafeAreaEdges AppletFrame::appliedSafeArea(SafeAreaEdges reach)
{
    if (!this->safeAreaManaged)
        return Box::appliedSafeArea(reach);
    // The sides, and the top or the bottom where no bar takes it
    bool headerShown = false, footerShown = false;
    for (View* child : this->getChildren())
    {
        headerShown |= isBar(child, HEADER_ID) && child->getVisibility() != Visibility::GONE;
        footerShown |= isBar(child, FOOTER_ID) && child->getVisibility() != Visibility::GONE;
    }
    SafeAreaEdges edges = SafeAreaEdges::HORIZONTAL;
    if (!headerShown)
        edges = edges | SafeAreaEdges::TOP;
    if (!footerShown)
        edges = edges | SafeAreaEdges::BOTTOM;
    return edges & reach;
}

SafeAreaEdges AppletFrame::childSafeAreaReach(View* child, SafeAreaEdges reach, SafeAreaEdges applied)
{
    if (!this->safeAreaManaged)
        return Box::childSafeAreaReach(child, reach, applied);
    // The bars take the top and the bottom the frame left them; the
    // content, between them, nothing
    if (isBar(child, HEADER_ID))
        return reach & SafeAreaEdges::TOP & ~applied;
    if (isBar(child, FOOTER_ID))
        return reach & SafeAreaEdges::BOTTOM & ~applied;
    return SafeAreaEdges::NONE;
}

void AppletFrame::setTitle(std::string title)
{
    this->title->setText(title);
}

void AppletFrame::pushContentView(View* view)
{
    contentViewStack.push_back(view);
    setContentView(view);
    Application::giveFocus(view);
}

void AppletFrame::popContentView(std::function<void(void)> cb)
{
    if (contentViewStack.size() <= 1)
    {
        if (!Application::popActivity(TransitionAnimation::FADE, cb))
        {
#ifndef IOS // Do not close the app in iOS
            auto dialog = new aurore::Dialog("hints/exit_hint"_i18n);
            dialog->addButton("hints/cancel"_i18n, []() {});
            dialog->addButton("hints/ok"_i18n, []()
                { Application::quit(); });
            dialog->open();
#endif
        }
        return;
    }

    View* lastView = contentViewStack.back();
    contentViewStack.pop_back();

    View* newView = contentViewStack.back();
    setContentView(newView);
    Application::giveFocus(newView);
    if (cb)
        cb();

    lastView->freeView();
}

void AppletFrame::popToRootContentView(std::function<void(void)> cb)
{
    while (contentViewStack.size() > 1)
    {
        popContentView();
    }
    cb();
}

void AppletFrame::setContentView(View* view)
{
    if (this->contentView)
    {
        // Remove the node
        this->removeView(this->contentView, false);
        this->contentView = nullptr;
    }

    if (!view)
        return;

    this->contentView = view;

    this->contentView->setDimensions(View::AUTO, View::AUTO);
    this->contentView->setGrow(1.0f);

    this->addView(this->contentView, 1);

    this->updateAppletFrameItem();
}

void AppletFrame::handleXMLElement(tinyxml2::XMLElement* element)
{
    if (this->contentView)
        fatal("aurore:AppletFrame can only have one child XML element");

    View* view = View::createFromXMLElement(element);
    contentViewStack.push_back(view);
    setContentView(view);
}

void AppletFrame::updateAppletFrameItem()
{
    hintBox->clearViews(false);

    setTitle(contentView->getAppletFrameItem()->title);
    setIcon(contentView->getAppletFrameItem()->iconPath);

    if (contentView->getAppletFrameItem()->getHintView())
        hintBox->addView(contentView->getAppletFrameItem()->getHintView());
}

void AppletFrame::setHeaderStyle(HeaderStyle style)
{
    this->style = style;

    Style appStyle = Application::getStyle();
    switch (style)
    {
        case HeaderStyle::REGULAR:
            header->setHeight(appStyle["aurore/applet_frame/header_height"]);
            title->setFontSize(appStyle["aurore/applet_frame/header_title_font_size"]);
            break;
        case HeaderStyle::POPUP:
            //            header->setHeight(appStyle["aurore/applet_frame/dropdown_header_height"]);
            //            title->setFontSize(appStyle["aurore/applet_frame/dropdown_header_title_font_size"]);
            break;
        default:
            break;
    }
}

View* AppletFrame::create()
{
    return new AppletFrame();
}

} // namespace aurore

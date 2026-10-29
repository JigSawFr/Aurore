// SPDX-FileCopyrightText: borealis contributors
// SPDX-License-Identifier: Apache-2.0

//
//  dropdown.cpp
//  aurore
//
//  Created by Даниил Виноградов on 14.05.2021.
//

#include "aurore/views/dropdown.hpp"

#include "aurore/core/application.hpp"
#include "aurore/views/cells/cell_radio.hpp"

namespace aurore
{

const std::string dropdownFrameXML = R"xml(
<aurore:Box
    width="1280"
    height="auto"
    axis="column">

    <aurore:AppletFrame
        id="aurore/dropdown/applet"
        width="auto"
        height="auto"
        grow="1"
        headerHidden="true"
        footerHidden="true"
        backgroundColor="@theme/aurore/backdrop">

        <aurore:Box
            width="auto"
            height="auto"
            grow="1"
            axis="column"
            justifyContent="flexEnd">

            <aurore:Box
                id="aurore/dropdown/content"
                width="auto"
                height="330"
                axis="column"
                backgroundColor="@theme/aurore/background">

                <aurore:Box
                    id="aurore/dropdown/header"
                    width="auto"
                    height="@style/aurore/dropdown/header_height"
                    axis="row"
                    paddingTop="@style/aurore/applet_frame/header_padding_top_bottom"
                    paddingBottom="@style/aurore/applet_frame/header_padding_top_bottom"
                    paddingLeft="@style/aurore/applet_frame/header_padding_sides"
                    paddingRight="@style/aurore/applet_frame/header_padding_sides"
                    marginLeft="@style/aurore/applet_frame/padding_sides"
                    marginRight="@style/aurore/applet_frame/padding_sides"
                    lineColor="@theme/aurore/applet_frame/separator"
                    lineBottom="1px">

                    <aurore:Label
                        id="aurore/dropdown/title_label"
                        width="auto"
                        height="auto"
                        marginTop="@style/aurore/applet_frame/header_title_top_offset"
                        fontSize="@style/aurore/dropdown/header_title_font_size" />

                </aurore:Box>
            
                <aurore:Box
                    width="auto"
                    height="auto"
                    axis="row"
                    grow="1"
                    justifyContent="center"
                    alignItems="stretch">
                    
                    <aurore:RecyclerFrame
                        id="aurore/dropdown/recycler"
                        width="100%"
                        height="auto"
                        paddingTop="@style/aurore/dropdown/listPadding"
                        paddingRight="@style/aurore/dropdown/listPaddingSides"
                        paddingBottom="@style/aurore/dropdown/listPadding"
                        paddingLeft="@style/aurore/dropdown/listPaddingSides"/>

                </aurore:Box>

            </aurore:Box>

        </aurore:Box>

    </aurore:AppletFrame>
    <aurore:BottomBar
        backgroundColor="@theme/aurore/background"/>
</aurore:Box>
)xml";

float min(float a, float b)
{
    if (a < b)
        return a;
    return b;
}

Dropdown::Dropdown(std::string title, std::vector<std::string> values, ValueSelectedEvent::Callback cb, int selected, ValueSelectedEvent::Callback dismissCb)
    : cb(cb)
    , dismissCb(dismissCb)
    , values(values)
    , selected(selected)
{
    this->inflateFromXMLString(dropdownFrameXML);
    content->setThemeArea(ThemeArea::DIALOG);
    this->title->setText(title);

    recycler->estimatedRowHeight = Application::getStyle()["aurore/dropdown/listItemHeight"];
    recycler->registerCell("Cell", []()
        {
        RadioCell* cell = new RadioCell();
        cell->setHeight(Application::getStyle()["aurore/dropdown/listItemHeight"]);
        cell->title->setFontSize(Application::getStyle()["aurore/dropdown/listItemTextSize"]);
        return cell; });
    recycler->setDefaultCellFocus(IndexPath(0, selected));
    recycler->setDataSource(this, false);

    Style style = Application::getStyle();

    float height = numberOfRows(recycler, 0) * style["aurore/dropdown/listItemHeight"]
        + header->getHeight()
        + style["aurore/dropdown/listPadding"] // top
        + style["aurore/dropdown/listPadding"] // bottom
        ;

    content->setHeight(min(height, Application::contentHeight * 0.73f));
}

int Dropdown::numberOfRows(RecyclerFrame* recycler, int section)
{
    return (int)values.size();
}

RecyclerCell* Dropdown::cellForRow(RecyclerFrame* recycler, IndexPath index)
{
    RadioCell* cell = (RadioCell*)recycler->dequeueReusableCell("Cell");
    cell->title->setText(values[index.row]);
    cell->setSelected(index.row == selected);
    return cell;
}

void Dropdown::didSelectRowAt(RecyclerFrame* recycler, IndexPath index)
{
    if (this->cb)
        this->cb(index.row);
    Application::popActivity(TransitionAnimation::FADE, [this, index]
        { if (this->dismissCb) this->dismissCb(index.row); });
}

AppletFrame* Dropdown::getAppletFrame()
{
    return applet;
}

void Dropdown::show(std::function<void(void)> cb, bool animate, float animationDuration)
{
    // Reduced motion: it fades in where it stays, without rising
    if (animate && !Application::getReduceMotion())
    {
        content->setTranslationY(30.0f);

        showOffset.stop();
        showOffset.reset(30.0f);
        showOffset.addStep(0, animationDuration, EasingFunction::quadraticOut);
        showOffset.setTickCallback([this]
            { this->offsetTick(); });
        showOffset.start();
    }

    Box::show(cb, animate, animationDuration);

    if (animate)
    {
        alpha.stop();
        alpha.reset(1);

        applet->alpha.stop();
        applet->alpha.reset(0);
        applet->alpha.addStep(1, animationDuration, EasingFunction::quadraticOut);
        applet->alpha.start();
    }
}

void Dropdown::hide(std::function<void(void)> cb, bool animated, float animationDuration)
{

    if (animated)
    {
        alpha.stop();
        alpha.reset(0);

        applet->alpha.stop();
        applet->alpha.reset(1);
        applet->alpha.addStep(0, animationDuration, EasingFunction::quadraticOut);
        applet->alpha.start();
    }

    Box::hide(cb, animated, animationDuration);
}

View* Dropdown::getParentNavigationDecision(View* from, View* newFocus, FocusDirection direction)
{
    View* result = Box::getParentNavigationDecision(from, newFocus, direction);

    RecyclerCell* cell = dynamic_cast<RecyclerCell*>(result);
    if (cell && cell != from)
    {
        cellFocusDidChangeEvent.fire(cell);
    }

    return result;
}

float Dropdown::getShowAnimationDuration(TransitionAnimation animation)
{
    return View::getShowAnimationDuration(animation) / 2;
}

void Dropdown::offsetTick()
{
    content->setTranslationY(showOffset);
}

} // namespace aurore

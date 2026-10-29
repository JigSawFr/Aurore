// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#include <cmath>
#include <ctime>
#include <aurore/core/activity.hpp>
#include <aurore/core/application.hpp>
#include <aurore/core/time.hpp>
#include <aurore/core/top_screen.hpp>

namespace aurore
{

namespace
{
    const std::string TOP_SCREEN_HEADER_XML = R"xml(
<aurore:Box
    width="auto"
    height="auto"
    axis="column">

    <aurore:Box
        width="auto"
        height="64"
        axis="row"
        alignItems="center"
        paddingLeft="24"
        paddingRight="24"
        lineColor="@theme/aurore/applet_frame/separator"
        lineBottom="1px">

        <aurore:Label
            id="aurore/top_screen/title"
            width="auto"
            height="auto"
            grow="1"
            shrink="1"
            singleLine="true"
            fontSize="26" />

        <aurore:Battery
            id="aurore/battery"
            marginLeft="16" />

        <aurore:Wireless
            id="aurore/wireless"
            marginLeft="16" />
    </aurore:Box>

    <aurore:Box
        width="auto"
        height="auto"
        grow="1"
        axis="column"
        justifyContent="center"
        alignItems="center">

        <aurore:Label
            id="aurore/top_screen/time"
            width="auto"
            height="auto"
            fontSize="96" />
    </aurore:Box>
</aurore:Box>
)xml";

    // The screen of the activities that give none, made when first drawn and
    // kept (as the debug layer is)
    TopScreenHeader* defaultHeader = nullptr;
} // namespace

void TopScreen::setSize(int width, int height)
{
    TopScreen::width  = std::max(0, width);
    TopScreen::height = std::max(0, height);
    Application::requestRedraw();
}

float TopScreen::getContentWidth()
{
    return Application::windowScale > 0 ? width / Application::windowScale : (float)width;
}

float TopScreen::getContentHeight()
{
    return Application::windowScale > 0 ? height / Application::windowScale : (float)height;
}

std::string TopScreen::getTitle()
{
    const auto stack = Application::getActivitiesStack();
    if (!stack.empty())
        if (auto* label = dynamic_cast<Label*>(stack.back()->getView("aurore/applet_frame/title_label")))
            if (!label->getFullText().empty())
                return label->getFullText();
    return Application::getTitle();
}

void TopScreen::draw(NVGcontext* vg, Rect rect)
{
    if (!TopScreen::isAvailable() || rect.getWidth() <= 0 || rect.getHeight() <= 0)
        return;

    const auto stack = Application::getActivitiesStack();
    View* view       = stack.empty() ? nullptr : stack.back()->getTopScreenView();
    if (!view)
    {
        if (!defaultHeader)
            defaultHeader = new TopScreenHeader();
        view = defaultHeader;
    }

    // Laid out again when the screen or the scale changed since
    float contentWidth  = TopScreen::getContentWidth();
    float contentHeight = TopScreen::getContentHeight();
    if (std::abs(view->getWidth() - contentWidth) > 0.5f || std::abs(view->getHeight() - contentHeight) > 0.5f)
        view->setDimensions(contentWidth, contentHeight);

    FrameContext ctx;
    ctx.vg         = vg;
    ctx.pixelRatio = rect.getWidth() / rect.getHeight();
    ctx.fontStash  = &Application::fontStash;
    ctx.theme      = Application::getTheme();

    nvgSave(vg);
    nvgScissor(vg, rect.getMinX(), rect.getMinY(), rect.getWidth(), rect.getHeight());
    nvgTranslate(vg, rect.getMinX(), rect.getMinY());
    float scale = rect.getWidth() / contentWidth;
    nvgScale(vg, scale, scale);

    nvgBeginPath(vg);
    nvgRect(vg, 0, 0, contentWidth, contentHeight);
    nvgFillColor(vg, ctx.theme["aurore/background"]);
    nvgFill(vg);

    view->frame(&ctx);
    nvgRestore(vg);
}

TopScreenHeader::TopScreenHeader()
{
    this->inflateFromXMLString(TOP_SCREEN_HEADER_XML);
    this->title = dynamic_cast<Label*>(this->getView("aurore/top_screen/title"));
    this->time  = dynamic_cast<Label*>(this->getView("aurore/top_screen/time"));

    Platform* platform = Application::getPlatform();
    if (View* battery = this->getView("aurore/battery"))
        battery->setVisibility(platform->canShowBatteryLevel() ? Visibility::VISIBLE : Visibility::GONE);
    if (View* wireless = this->getView("aurore/wireless"))
        wireless->setVisibility(platform->canShowWirelessLevel() ? Visibility::VISIBLE : Visibility::GONE);
}

void TopScreenHeader::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    std::string title = TopScreen::getTitle();
    if (this->title && title != this->shownTitle)
    {
        this->title->setText(title);
        this->shownTitle = title;
    }

    char clock[16] = "";
    std::tm local  = localTime(getWallTime()); // the harness' own in the tests
    std::strftime(clock, sizeof(clock), "%H:%M", &local);
    if (this->time && clock != this->shownTime)
    {
        this->time->setText(clock);
        this->shownTime = clock;
    }

    Box::draw(vg, x, y, width, height, style, ctx);
}

} // namespace aurore

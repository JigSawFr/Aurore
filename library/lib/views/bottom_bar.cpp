/*
    Copyright 2021 XITRIX

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

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <aurore/views/bottom_bar.hpp>
#include <aurore/views/hint.hpp>

#ifdef PS4
#include <aurore/platforms/ps4/ps4_sysmodule.hpp>
#endif

namespace aurore
{

const std::string bottomBarXML = R"xml(

<aurore:Box
    width="auto"
    height="@style/aurore/applet_frame/footer_height"
    axis="column">
    <aurore:Box
        width="auto"
        height="@style/aurore/applet_frame/footer_height"
        marginLeft="@style/aurore/hints/footer_margin_sides"
        marginRight="@style/aurore/hints/footer_margin_sides"
        paddingLeft="@style/aurore/hints/footer_padding_sides"
        paddingRight="@style/aurore/hints/footer_padding_sides"
        lineColor="@theme/aurore/applet_frame/separator"
        lineTop="1px"
        alignItems="stretch">
        <aurore:Box
            width="auto"
            height="@style/aurore/applet_frame/footer_height"
            axis="row"
            grow="1"
            direction="rightToLeft"
            justifyContent="spaceBetween"
            paddingTop="@style/aurore/hints/footer_padding_top_bottom"
            paddingBottom="@style/aurore/hints/footer_padding_top_bottom" >

            <aurore:Hints
                width="auto"
                height="auto"/>

            <!-- The clock, battery and Wi-Fi: the hints' room on a compact screen -->
            <aurore:Box
                width="auto"
                height="auto"
                axis="row"
                alignItems="center"
                direction="leftToRight"
                compact:visibility="gone" >

                <aurore:Battery
                    id="aurore/battery"
                    marginRight="21"
                    marginBottom="5"/>

                <aurore:Wireless
                    id="aurore/wireless"
                    marginRight="21"
                    marginBottom="5"/>

                <aurore:Label
                    id="aurore/hints/time"
                    width="auto"
                    height="auto"
                    verticalAlign="center"
                    fontSize="21.5" />

            </aurore:Box>

        </aurore:Box>
    </aurore:Box>
</aurore:Box>
)xml";

BottomBar::BottomBar()
{
    this->inflateFromXMLString(bottomBarXML);
    // Little room to grow: the clock as large as the hints.
    time->setFontScaleMax(getHintsFontScaleMax());

    Platform* platform = Application::getPlatform();
    battery->setVisibility(platform->canShowBatteryLevel() ? Visibility::VISIBLE : Visibility::GONE);
    wireless->setVisibility(platform->canShowWirelessLevel() ? Visibility::VISIBLE : Visibility::GONE);
}

void BottomBar::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    this->updateText();
    Box::draw(vg, x, y, width, height, style, ctx);
}

void BottomBar::updateText()
{
#ifdef PS4
    OrbisDateTime lt {};
    if (sceRtcGetCurrentClockLocalTime)
        sceRtcGetCurrentClockLocalTime(&lt);
    tm tm {};
    tm.tm_hour = lt.hour;
    tm.tm_min  = lt.minute;
    tm.tm_sec  = lt.second;
#else
    std::time_t in_time_t = getWallTime();
    auto tm               = localTime(in_time_t);
#endif
    std::stringstream ss;
    ss << std::put_time(&tm, "%H:%M");
    if (ss.str() != bottomText)
    {
        bottomText = ss.str();
        if (Application::getFPSStatus())
        {
            time->setText(bottomText + " | FPS:" + std::to_string(Application::getFPS()));
        }
        else
        {
            time->setText(bottomText);
        }
    }
}

View* BottomBar::create()
{
    return new BottomBar();
}

} // namespace aurore

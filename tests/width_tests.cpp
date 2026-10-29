/*
    Copyright 2026 JigSawFr

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

// The classes of width: what the screen's width in layout units gives, and
// the XML attributes for one class of screen (compact:width="…").

#include <yoga/Yoga.h>
#include <aurore.hpp>
#include <aurore/test/harness.hpp>

#include <exception>
#include <string>

using aurore::test::Harness;

namespace
{

// A design width for the length of a test (a small screen's), the usual one
// again after it
struct DesignWidth
{
    explicit DesignWidth(uint32_t width, uint32_t height)
    {
        aurore::Application::ORIGINAL_WINDOW_WIDTH  = width;
        aurore::Application::ORIGINAL_WINDOW_HEIGHT = height;
    }
    ~DesignWidth()
    {
        aurore::Application::ORIGINAL_WINDOW_WIDTH  = 1280;
        aurore::Application::ORIGINAL_WINDOW_HEIGHT = 720;
    }
};

float styleWidth(aurore::View* view)
{
    return YGNodeStyleGetWidth(view->getYGNode()).value;
}

const char* const LAYOUT = "<aurore:Box width=\"400\" compact:width=\"100\" expanded:height=\"50\" medium:height=\"60\" "
                           "compact:focus:cornerRadius=\"4\"/>";

} // namespace

AURORE_TEST(width_classes_follow_the_layout_width)
{
    AURORE_CHECK(aurore::widthClassOf(320) == aurore::WidthClass::COMPACT);
    AURORE_CHECK(aurore::widthClassOf(719) == aurore::WidthClass::COMPACT);
    AURORE_CHECK(aurore::widthClassOf(720) == aurore::WidthClass::MEDIUM);
    AURORE_CHECK(aurore::widthClassOf(1079) == aurore::WidthClass::MEDIUM);
    AURORE_CHECK(aurore::widthClassOf(1280) == aurore::WidthClass::EXPANDED);

    Harness harness;
    AURORE_CHECK(aurore::Application::getWidthClass() == aurore::WidthClass::EXPANDED);
}

AURORE_TEST(width_attributes_apply_on_their_class_only)
{
    Harness harness;
    aurore::View* wide = aurore::View::createFromXMLString(LAYOUT);
    AURORE_CHECK_EQ(styleWidth(wide), 400.0f);
    AURORE_CHECK_EQ(YGNodeStyleGetHeight(wide->getYGNode()).value, 50.0f);
    delete wide;
}

AURORE_TEST(width_attributes_win_on_a_small_screen_whatever_their_order)
{
    DesignWidth small(480, 360);
    Harness harness;
    AURORE_CHECK_EQ(aurore::Application::contentWidth, 480.0f);
    AURORE_CHECK(aurore::Application::getWidthClass() == aurore::WidthClass::COMPACT);

    aurore::View* narrow = aurore::View::createFromXMLString(LAYOUT);
    AURORE_CHECK_EQ(styleWidth(narrow), 100.0f);
    // No height for a compact screen: the yoga default
    AURORE_CHECK(YGNodeStyleGetHeight(narrow->getYGNode()).unit == YGUnitAuto);
    delete narrow;

    aurore::View* before = aurore::View::createFromXMLString("<aurore:Box compact:width=\"100\" width=\"400\"/>");
    AURORE_CHECK_EQ(styleWidth(before), 100.0f);
    delete before;
}

AURORE_TEST(width_attributes_are_checked_on_every_screen)
{
    Harness harness;
    for (const char* xml : { "<aurore:Box compact:widht=\"100\"/>", "<aurore:Box narrow:width=\"100\"/>" })
    {
        bool refused = false;
        try
        {
            delete aurore::View::createFromXMLString(xml);
        }
        catch (const std::exception&)
        {
            refused = true;
        }
        AURORE_CHECK(refused);
    }
}

int main(int argc, char** argv)
{
    return aurore::test::runTests(argc, argv);
}

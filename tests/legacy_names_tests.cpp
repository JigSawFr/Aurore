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

// legacy-names: file
// The names borealis gave the library still work, deprecated, until Aurore
// 1.0 (docs/migrating-from-borealis.md): the brls namespace, the
// <borealis/...> headers, the BRLS_ and BOREALIS_ macros and environment
// variables, the brls: XML tags and the brls/ theme and style keys. Written
// as an app that has not moved yet; the old CMake options are built by
// tools/ci_variant.sh legacy.

#include <borealis.hpp>
#include <borealis/extern/nlohmann/json.hpp>
#include <borealis/platforms/desktop/desktop_platform.hpp>
#include <borealis/test/harness.hpp>

#include <cstdlib>
#include <cstring>
#include <type_traits>

// The deprecated names warn where they are used: not here, where they are the point
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

using brls::test::Harness;
using namespace brls::literals;

// The build's flags, under the old names too
#ifndef BOREALIS_USE_OPENGL
#error "BOREALIS_USE_OPENGL is not defined"
#endif
#ifndef BRLS_RESOURCES
#error "BRLS_RESOURCES is not defined"
#endif

static_assert(std::is_same_v<brls::View, aurore::View>);
static_assert(std::is_same_v<brls::BrlsKeyCombination, aurore::AuroreKeyCombination>);
static_assert(std::is_same_v<brls::BrlsKeyboardScancode, aurore::AuroreKeyboardScancode>);
static_assert(brls::BRLS_KBD_KEY_Y == aurore::AURORE_KBD_KEY_Y);
static_assert(brls::BRLS_KBD_MODIFIER_CTRL == aurore::AURORE_KBD_MODIFIER_CTRL);

static bool sameColor(NVGcolor a, NVGcolor b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

// A forward declaration opens the namespace: the new name only (an alias
// cannot be opened again)
namespace aurore
{
class Activity;
}
static_assert(std::is_same_v<brls::Activity, aurore::Activity>);

// What an app wrote with borealis' names: a class of its own and its XML

class LegacyBox : public brls::Box
{
  public:
    LegacyBox()
    {
        this->inflateFromXMLString(R"xml(
            <brls:Box width="200" height="100" axis="column">
                <brls:Label id="legacy/label" text="borealis" textColor="@theme/brls/text"/>
                <brls:Rectangle id="legacy/rect" width="10" height="10" color="@theme/brls/accent"/>
            </brls:Box>)xml");
    }

    BRLS_BIND(brls::Label, label, "legacy/label");
    BRLS_BIND(brls::Rectangle, rect, "legacy/rect");
};

BRLS_TEST(old_namespace_and_headers_are_the_new_ones)
{
    Harness h;
    brls::Box* box     = new aurore::Box();
    aurore::View* view = box;
    BRLS_CHECK(dynamic_cast<brls::Box*>(view) == box);
    delete box;
    BRLS_CHECK_EQ(std::string("hints/back"_i18n), aurore::getStr("hints/back"));
    nlohmann::json json = nlohmann::json::parse(R"({"a": 1})");
    BRLS_CHECK_EQ(json["a"].get<int>(), 1);
    BRLS_CHECK_EQ(std::string(BRLS_ASSET("xml/x.xml")), std::string(AURORE_ASSET("xml/x.xml")));
}

BRLS_TEST(old_key_combination_registers_an_action)
{
    Harness h;
    brls::Box* box = new brls::Box();
    brls::BrlsKeyCombination combination(brls::BRLS_KBD_KEY_Y);
    BRLS_CHECK_EQ((int)combination.code, (int)aurore::AURORE_KBD_KEY_Y);
    box->registerAction(combination, [](brls::View*)
        { return true; });
    delete box;
}

BRLS_TEST(old_environment_variables_are_still_read)
{
    for (const char* name : { "AURORE_THEME", "BRLS_THEME", "BOREALIS_THEME", "AURORE_LANG", "BRLS_LANG", "BOREALIS_LANG" })
        unsetenv(name);
    BRLS_CHECK(aurore::environmentVariable("THEME") == nullptr);

    // BOREALIS_ is borealis' oldest, BRLS_ comes before it, AURORE_ before both
    setenv("BOREALIS_THEME", "LIGHT", 1);
    BRLS_CHECK_EQ(std::string(brls::DesktopPlatform::environment("THEME")), std::string("LIGHT"));
    setenv("BRLS_THEME", "DARK", 1);
    BRLS_CHECK_EQ(std::string(brls::DesktopPlatform::environment("THEME")), std::string("DARK"));
    setenv("AURORE_THEME", "LIGHT", 1);
    BRLS_CHECK_EQ(std::string(brls::DesktopPlatform::environment("THEME")), std::string("LIGHT"));

    std::string locale;
    setenv("BOREALIS_LANG", "de_DE.UTF-8", 1);
    BRLS_CHECK(brls::DesktopPlatform::localeFromEnvironmentVariable(&locale));
    BRLS_CHECK_EQ(locale, std::string("de"));
    setenv("BRLS_LANG", "fr-CA", 1);
    BRLS_CHECK(brls::DesktopPlatform::localeFromEnvironmentVariable(&locale));
    BRLS_CHECK_EQ(locale, std::string("fr-CA"));

    // The test harness' own
    setenv("BRLS_TEST_UPDATE", "1", 1);
    BRLS_CHECK_EQ(std::string(aurore::environmentVariable("TEST_UPDATE")), std::string("1"));

    for (const char* name : { "AURORE_THEME", "BRLS_THEME", "BOREALIS_THEME", "BRLS_LANG", "BOREALIS_LANG", "BRLS_TEST_UPDATE" })
        unsetenv(name);
}

BRLS_TEST(old_xml_tags_are_still_read)
{
    Harness h;
    LegacyBox* box = new LegacyBox();
    BRLS_CHECK_EQ(box->label->getFullText(), std::string("borealis"));
    BRLS_CHECK(dynamic_cast<aurore::Rectangle*>(box->getView("legacy/rect")) != nullptr);

    // A view an app registers under the old prefix is the new one's, both ways
    brls::Application::registerXMLView("brls:LegacyRectangle", aurore::Rectangle::create);
    BRLS_CHECK(aurore::Application::XMLViewsRegisterContains("aurore:LegacyRectangle"));
    BRLS_CHECK(aurore::Application::XMLViewsRegisterContains("brls:Box"));
    aurore::View* made = aurore::View::createFromXMLString(R"xml(<aurore:LegacyRectangle width="4" height="4"/>)xml");
    BRLS_CHECK(dynamic_cast<aurore::Rectangle*>(made) != nullptr);
    delete made;

    // The children a tab frame reads itself
    aurore::View* tabs = aurore::View::createFromXMLString(R"xml(
        <brls:TabFrame>
            <brls:Tab label="One"><brls:Box/></brls:Tab>
            <brls:Separator/>
            <aurore:Tab label="Two"/>
        </brls:TabFrame>)xml");
    BRLS_CHECK(dynamic_cast<aurore::TabFrame*>(tabs) != nullptr);
    delete tabs;
    delete box;
}

BRLS_TEST(old_theme_and_style_keys_are_still_read)
{
    Harness h;
    aurore::Theme theme = aurore::Application::getTheme();
    BRLS_CHECK(theme.hasColor("brls/text"));
    BRLS_CHECK(sameColor(theme.getColor("brls/text"), theme.getColor("aurore/text")));
    BRLS_CHECK(sameColor(theme["brls/accent"], theme["aurore/accent"]));
    theme.addColor("brls/legacy_test", nvgRGB(1, 2, 3));
    BRLS_CHECK(sameColor(theme.getColor("aurore/legacy_test"), nvgRGB(1, 2, 3)));

    aurore::Style style = aurore::Application::getStyle();
    BRLS_CHECK(style.hasMetric("brls/notification/width"));
    BRLS_CHECK_EQ(style.getMetric("brls/notification/width"), style.getMetric("aurore/notification/width"));
    style.addMetric("brls/legacy_test", 42.0f);
    BRLS_CHECK_EQ(style["aurore/legacy_test"], 42.0f);

    // A theme file written with the old keys
    BRLS_CHECK(aurore::Theme::load("name = Legacy\nbrls/backdrop = #102030\n[metrics]\nbrls/notification/width = 321\n"));
    h.step(1);
    BRLS_CHECK(sameColor(aurore::Application::getTheme().getColor("aurore/backdrop"), nvgRGB(0x10, 0x20, 0x30)));
    BRLS_CHECK_EQ(aurore::Application::getStyle().getMetric("aurore/notification/width"), 321.0f);
}

int main(int argc, char** argv)
{
    return brls::test::runTests(argc, argv);
}

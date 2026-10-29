// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// The density: the lengths of the style, `dp` in XML and aurore::dp() follow
// it; the text, the durations and the plain numbers do not.

#include <yoga/Yoga.h>
#include <aurore.hpp>
#include <aurore/cadran/ui/cadran_kit.hpp>
#include <aurore/test/harness.hpp>

#include <cmath>
#include <limits>
#include <string>

using aurore::test::Harness;

namespace
{

// The density for the length of a test, the default one again after it
struct Density
{
    explicit Density(float density) { aurore::setDensity(density); }
    ~Density() { aurore::setDensity(aurore::DENSITY_COMFORTABLE); }
};

bool near(float actual, float expected)
{
    return std::fabs(actual - expected) < 0.001f;
}

float styleWidth(aurore::View* view)
{
    return YGNodeStyleGetWidth(view->getYGNode()).value;
}

} // namespace

AURORE_TEST(density_scales_the_lengths_of_the_style_only)
{
    Harness harness;
    aurore::Style style = aurore::getStyle();
    float padding       = style.getValues()->getRawMetric("aurore/button/padding_sides");
    float text          = style["aurore/button/text_size"];
    float show          = style["aurore/animations/show"];
    AURORE_CHECK_EQ(aurore::getDensity(), aurore::DENSITY_COMFORTABLE);
    AURORE_CHECK(near(style["aurore/button/padding_sides"], padding));

    Density dense(aurore::DENSITY_DENSE);
    AURORE_CHECK(near(style["aurore/button/padding_sides"], padding * 0.7f));
    AURORE_CHECK(near(style["aurore/button/text_size"], text));
    AURORE_CHECK(near(style["aurore/animations/show"], show));
    AURORE_CHECK(near(aurore::dp(20), 14));
    AURORE_CHECK(near(aurore::Application::getDensity(), 0.7f));
}

AURORE_TEST(density_keeps_the_kind_of_a_metric_changed)
{
    Harness harness;
    aurore::Style style = aurore::getStyle();
    // A length the app changes stays a length; a new metric is not one unless
    // added as such
    style.addMetric("aurore/button/padding_sides", 30);
    style.addMetric("test/density/ratio", 3);
    style.addLength("test/density/gap", 10);
    AURORE_CHECK(style.getValues()->getKind("aurore/button/padding_sides") == aurore::MetricKind::LENGTH);
    AURORE_CHECK(style.getValues()->getKind("test/density/ratio") == aurore::MetricKind::OTHER);
    AURORE_CHECK(style.getValues()->getKind("test/density/unknown") == aurore::MetricKind::OTHER);

    Density compact(aurore::DENSITY_COMPACT);
    AURORE_CHECK(near(style["aurore/button/padding_sides"], 25.5f));
    AURORE_CHECK(near(style["test/density/ratio"], 3));
    AURORE_CHECK(near(style["test/density/gap"], 8.5f));
    style.addMetric("aurore/button/padding_sides", 25);
}

AURORE_TEST(density_applies_to_dp_in_xml)
{
    Harness harness;
    Density dense(aurore::DENSITY_DENSE);
    aurore::View* box = aurore::View::createFromXMLString(
        "<aurore:Box width=\"40dp\" height=\"40px\" cornerRadius=\"10dp\" disabled:cornerRadius=\"20dp\"/>");
    AURORE_CHECK(near(styleWidth(box), 28));
    AURORE_CHECK(near(YGNodeStyleGetHeight(box->getYGNode()).value, 40));
    AURORE_CHECK(near(box->getCornerRadius(), 7));
    // A state style in dp: applied at the next frame
    box->setStyleState(aurore::StyleState::DISABLED, true);
    harness.push(new aurore::Activity(box));
    harness.settle();
    AURORE_CHECK(near(box->getCornerRadius(), 14));

    aurore::View* styled = aurore::View::createFromXMLString("<aurore:Box width=\"@style/aurore/button/padding_sides\"/>");
    AURORE_CHECK(near(styleWidth(styled), aurore::getStyle().getValues()->getRawMetric("aurore/button/padding_sides") * 0.7f));
    delete styled;

    for (const char* value : { "dp", "nandp", "1e99dp", "xdp" })
    {
        bool refused = false;
        try
        {
            delete aurore::View::createFromXMLString(std::string("<aurore:Box width=\"") + value + "\"/>");
        }
        catch (const std::exception&)
        {
            refused = true;
        }
        AURORE_CHECK(refused);
    }
}

AURORE_TEST(density_is_kept_in_range_and_read_by_name)
{
    Harness harness;
    Density reset(aurore::DENSITY_COMFORTABLE);
    aurore::setDensity(0.1f);
    AURORE_CHECK_EQ(aurore::getDensity(), aurore::DENSITY_MIN);
    aurore::setDensity(9);
    AURORE_CHECK_EQ(aurore::getDensity(), aurore::DENSITY_MAX);
    aurore::setDensity(std::numeric_limits<float>::quiet_NaN());
    AURORE_CHECK_EQ(aurore::getDensity(), aurore::DENSITY_MAX);

    float density = 0;
    AURORE_CHECK(aurore::parseDensity("comfortable", &density) && density == aurore::DENSITY_COMFORTABLE);
    AURORE_CHECK(aurore::parseDensity("compact", &density) && density == aurore::DENSITY_COMPACT);
    AURORE_CHECK(aurore::parseDensity("dense", &density) && density == aurore::DENSITY_DENSE);
    AURORE_CHECK(aurore::parseDensity("0.8", &density) && near(density, 0.8f));
    for (const char* text : { "", "loose", "0.8x", "nan", "1e99" })
        AURORE_CHECK(!aurore::parseDensity(text, &density));
}

AURORE_TEST(density_reaches_cadran_kit)
{
    Harness harness;
    Density dense(aurore::DENSITY_DENSE);
    // A card's padding and corner, an icon's size: Cadran's own numbers
    aurore::Box* card = cadran::kit::card(false, false);
    AURORE_CHECK(near(YGNodeStyleGetPadding(card->getYGNode(), YGEdgeLeft).value, 24 * 0.7f));
    AURORE_CHECK(near(card->getCornerRadius(), cadran::kit::RADIUS_CARD * 0.7f));
    aurore::View* icon = cadran::kit::icon("check", 20, "cadran/text");
    AURORE_CHECK(near(styleWidth(icon), 14));
    aurore::Box* chip = cadran::kit::chip("15 min", 40);
    AURORE_CHECK(near(YGNodeStyleGetHeight(chip->getYGNode()).value, 28));
    delete card;
    delete icon;
    delete chip;
}

int main(int argc, char** argv)
{
    return aurore::test::runTests(argc, argv);
}

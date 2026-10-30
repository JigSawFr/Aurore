// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#include "view/top_screens.hpp"

#include <aurore/cadran.hpp>

#include "demo_app.hpp"

using namespace aurore::literals; // for _i18n

namespace kit = cadran::kit;

namespace demo::top
{

namespace
{
    // The night of a top screen: edge to edge, no ridges under the text
    void dress(AuroraView* night)
    {
        night->setRadius(0);
        night->setRidges(false);
        night->setPadding(28, 36, 28, 36);
    }

    // What a tab is about, in its own words (its first lead)
    std::string leadOf(int tab)
    {
        switch (tab)
        {
            case TAB_HOME:
                return "demo/home/pitch"_i18n;
            case TAB_LIBRARY:
                return aurore::getStr("demo/library/count", library::games().size());
            case TAB_COMPONENTS:
                return "demo/components/buttons_lead"_i18n;
            case TAB_MOTION:
                return "demo/motion/easings_lead"_i18n;
            case TAB_TEXT:
                return "demo/text/languages_lead"_i18n;
            case TAB_THEMES:
                return "demo/themes/variant_lead"_i18n;
            case TAB_INPUT:
                return "demo/input/controller_lead"_i18n;
            case TAB_DEVELOPERS:
                return "demo/developers/frame_lead"_i18n;
            case TAB_ABOUT:
                return "demo/about/description"_i18n;
            default:
                return "";
        }
    }
} // namespace

TabScreen::TabScreen()
{
    dress(this);
    this->setAxis(aurore::Axis::COLUMN);
    this->setJustifyContent(aurore::JustifyContent::CENTER);
    this->title = kit::display("", 44);
    this->title->setTextColor(AuroraView::SNOW);
    this->addView(this->title);
    this->lead = kit::label("", 22);
    this->lead->setTextColor(AuroraView::MIST);
    this->lead->setMarginTop(12);
    this->addView(this->lead);
}

void TabScreen::draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
    aurore::FrameContext* ctx)
{
    // The texts of the tab shown, set again when it changes: this root view
    // is laid out again at once, before its children are drawn
    int tab = demo::shownTab();
    if (tab != this->tab)
    {
        this->tab = tab;
        this->title->setText(tab == TAB_HOME ? std::string("Aurore") : aurore::getStr(std::string("demo/tabs/") + tabName(tab)));
        this->lead->setText(leadOf(tab));
    }
    AuroraView::draw(vg, x, y, width, height, style, ctx);
}

GameScreen::GameScreen()
{
    dress(this);
    this->setAxis(aurore::Axis::ROW);
    this->setAlignItems(aurore::AlignItems::CENTER);
    this->cover = new CoverArt();
    this->cover->setWidth(210);
    this->cover->setHeight(280);
    this->cover->setRadius(14);
    this->cover->setTitleShown(false);
    this->cover->setMarginRight(32);
    this->addView(this->cover);

    auto* texts = new aurore::Box(aurore::Axis::COLUMN);
    texts->setGrow(1);
    texts->setWidth(0);
    this->title = kit::display("", 34);
    this->title->setTextColor(AuroraView::SNOW);
    texts->addView(this->title);
    this->genre = kit::label("", 22);
    this->genre->setTextColor(AuroraView::MIST);
    this->genre->setMarginTop(10);
    texts->addView(this->genre);
    this->played = kit::label("", 22);
    this->played->setTextColor(AuroraView::MIST);
    this->played->setMarginTop(4);
    texts->addView(this->played);
    this->addView(texts);
}

void GameScreen::setGame(const library::Game* game)
{
    this->cover->setGame(game);
    if (!game)
        return;
    this->title->setText(game->title);
    this->genre->setText(library::genreName(game->genre) + "  ·  " + std::to_string(game->year));
    this->played->setText(game->minutes ? aurore::getStr("demo/library/played", library::playtime(game->minutes))
                                        : "demo/library/never"_i18n);
}

} // namespace demo::top

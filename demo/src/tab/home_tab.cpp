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

#include "tab/home_tab.hpp"

#include <aurore/cadran.hpp>

#include "demo_app.hpp"
#include "view/aurora_view.hpp"
#include "view/charts.hpp"
#include "view/showcase.hpp"

using namespace aurore::literals; // for _i18n

namespace kit = cadran::kit;

namespace
{
// A word on the night, in a thin pill: a platform
aurore::Box* platform(const std::string& name)
{
    auto* pill = new aurore::Box(aurore::Axis::ROW);
    pill->setPadding(5, 12, 5, 12);
    pill->setCornerRadius(15);
    pill->setBorderThickness(1);
    pill->setBorderColor(nvgRGBA(0xE8, 0xEE, 0xF7, 70));
    pill->setBackgroundColor(nvgRGBA(0x0B, 0x14, 0x24, 120));
    aurore::Label* label = kit::label(name, kit::TEXT_META);
    label->setTextColor(AuroraView::SNOW);
    label->setSingleLine(true);
    pill->addView(label);
    return pill;
}

// The night at the top: the mark, the name, the pitch, the platforms and
// two ways in
aurore::View* hero(aurore::View* tab)
{
    auto* night = new AuroraView();
    night->setPadding(34, 40, 34, 40);
    night->setMinHeight(300);

    auto* head = new aurore::Box(aurore::Axis::ROW);
    head->setAlignItems(aurore::AlignItems::CENTER);
    head->setGap(16);
    auto* mark = new aurore::Image();
    mark->setImageFromRes("img/aurore_96.png");
    mark->setWidth(56);
    mark->setHeight(56);
    mark->setCornerRadius(14);
    head->addView(mark);
    aurore::Label* name = kit::display("Aurore", 48);
    name->setTextColor(AuroraView::SNOW);
    head->addView(name);
    night->addView(head);

    aurore::Label* pitch = kit::label("demo/home/pitch"_i18n, 24);
    pitch->setTextColor(AuroraView::SNOW);
    pitch->setMarginTop(18);
    pitch->setMaxWidth(720);
    night->addView(pitch);
    aurore::Label* more = kit::label("demo/home/more"_i18n, kit::TEXT_BODY);
    more->setTextColor(AuroraView::MIST);
    more->setMarginTop(8);
    more->setMaxWidth(720);
    night->addView(more);

    aurore::Box* platforms = show::row(true, 8);
    platforms->setMarginTop(20);
    for (const char* name : { "Switch", "PS4", "PS Vita", "Android", "iOS", "Windows", "macOS", "Linux" })
        platforms->addView(platform(name));
    platforms->addView(platform("demo/home/web"_i18n));
    night->addView(platforms);

    aurore::Box* actions = show::row(true, 12);
    actions->setMarginTop(24);
    actions->addView(show::button("demo/home/explore"_i18n, [tab]
        { demo::openTab(tab, demo::TAB_LIBRARY); }, true));
    actions->addView(show::button("demo/home/components"_i18n, [tab]
        { demo::openTab(tab, demo::TAB_COMPONENTS); }));
    night->addView(actions);
    return night;
}

// A card of the dashboard: a kicker of its own, then what it shows
aurore::Box* dashCard(const char* icon, const std::string& title)
{
    aurore::Box* card = show::card();
    aurore::Box* head = kit::kicker(icon, title);
    head->setMarginBottom(16);
    card->addView(head);
    return card;
}

CadranDigits* figure(const std::string& text, float size)
{
    auto* digits = new CadranDigits();
    digits->setText(text);
    digits->setFontSize(size);
    digits->setSmallUnits(true);
    digits->setThemeColor("textColor", "cadran/text");
    digits->setFitParent(true);
    return digits;
}

// A figure and what it counts, side by side with the others
aurore::Box* fact(const std::string& value, const std::string& caption)
{
    aurore::Box* card = kit::top_card(true);
    card->setPadding(18, 22, 18, 22);
    CadranDigits* digits = figure(value, 40);
    digits->setHeight(50);
    card->addView(digits);
    aurore::Label* label = kit::label(caption, kit::TEXT_NOTE, "cadran/text2");
    card->addView(label);
    return card;
}

// A tile that opens a tab: an icon on a soft square, its name, a line
aurore::Box* link(aurore::View* tab, int position, const char* icon, const std::string& name,
    const std::string& line)
{
    aurore::Box* tile = show::card(true, true);
    tile->setPadding(18, 20, 18, 20);
    tile->setMinHeight(148);
    auto* square = new aurore::Box(aurore::Axis::ROW);
    square->setWidth(44);
    square->setHeight(44);
    square->setCornerRadius(kit::RADIUS_ICON);
    square->setJustifyContent(aurore::JustifyContent::CENTER);
    square->setAlignItems(aurore::AlignItems::CENTER);
    square->setThemeColor("backgroundColor", "cadran/teal_soft");
    square->addView(kit::icon(icon, 24, "cadran/teal"));
    square->setMarginBottom(14);
    tile->addView(square);
    tile->addView(kit::label(name, kit::TEXT_BODY + 1, "cadran/text"));
    tile->addView(show::note(line));
    tile->registerClickAction([tab, position](aurore::View*)
        {
        demo::openTab(tab, position);
        return true; });
    tile->addGestureRecognizer(new aurore::TapGestureRecognizer(tile));
    return tile;
}
} // namespace

HomeTab::HomeTab()
{
    aurore::Box* page = show::page(this);
    page->addView(hero(this));

    // A dashboard as an app would make one
    aurore::Box* dash  = show::section(page, "today", "demo/home/dashboard"_i18n, "demo/home/dashboard_lead"_i18n);
    aurore::Box* cards = show::row();
    dash->addView(cards);

    aurore::Box* ring = dashCard("shelf", "demo/home/collection"_i18n);
    auto* ringRow     = show::row(false, 20);
    ringRow->setAlignItems(aurore::AlignItems::CENTER);
    auto* gauge = new RingGauge(120, 12);
    gauge->setValue(0.72f);
    gauge->addView(figure("72%", 30));
    ringRow->addView(gauge);
    auto* ringText = show::column(4);
    ringText->setGrow(1);
    ringText->setWidth(0);
    ringText->addView(kit::label("demo/home/finished"_i18n, kit::TEXT_BODY, "cadran/text"));
    ringRow->addView(ringText);
    ring->addView(ringRow);
    aurore::Box* pill = kit::pill("demo/home/streak"_i18n, "cadran/teal");
    pill->setAlignSelf(aurore::AlignSelf::FLEX_START);
    pill->setMarginTop(16);
    ring->addView(pill);
    cards->addView(ring);

    aurore::Box* week = dashCard("timer", "demo/home/week"_i18n);
    auto* weekHead    = show::row(false, 12);
    weekHead->setAlignItems(aurore::AlignItems::FLEX_END);
    CadranDigits* hours = figure("14 h 20", 36);
    hours->setFitParent(false);
    weekHead->addView(hours);
    aurore::Label* trend = kit::label("demo/home/trend"_i18n, kit::TEXT_NOTE, "cadran/teal");
    trend->setMarginBottom(6);
    weekHead->addView(trend);
    week->addView(weekHead);
    auto* bars = new BarChart();
    bars->setValues({ 0.35f, 0.6f, 0.25f, 0.8f, 0.5f, 0.95f, 0.7f });
    bars->setHeight(72);
    bars->setMarginTop(14);
    week->addView(bars);
    cards->addView(week);

    aurore::Box* storage = dashCard("save", "demo/home/storage"_i18n);
    storage->addView(figure("41.2 GB", 36));
    storage->addView(show::note("demo/home/storage_note"_i18n));
    auto* bar = new kit::Bar();
    bar->set(0.64f);
    bar->setMarginTop(16);
    storage->addView(bar);
    aurore::Box* warn = kit::pill("demo/home/updates"_i18n, "cadran/amber");
    warn->setAlignSelf(aurore::AlignSelf::FLEX_START);
    warn->setMarginTop(16);
    storage->addView(warn);
    cards->addView(storage);

    // Aurore in figures
    aurore::Box* facts   = show::section(page, "sparkle", "demo/home/figures"_i18n);
    aurore::Box* factRow = show::row();
    factRow->addView(fact("15", "demo/home/languages"_i18n));
    factRow->addView(fact("9", "demo/home/platforms"_i18n));
    factRow->addView(fact("60+", "demo/home/icons"_i18n));
    factRow->addView(fact("15", "demo/home/presets"_i18n));
    facts->addView(factRow);

    // The other tabs
    aurore::Box* explore = show::section(page, "list", "demo/home/explore_title"_i18n);
    struct Link
    {
        int tab;
        const char* icon;
        const char* name;
        const char* line;
    };
    const Link links[] = {
        { demo::TAB_LIBRARY, "shelf", "demo/tabs/library", "demo/home/link_library" },
        { demo::TAB_COMPONENTS, "sparkle", "demo/tabs/components", "demo/home/link_components" },
        { demo::TAB_MOTION, "bolt", "demo/tabs/motion", "demo/home/link_motion" },
        { demo::TAB_TEXT, "language", "demo/tabs/text", "demo/home/link_text" },
        { demo::TAB_THEMES, "palette", "demo/tabs/themes", "demo/home/link_themes" },
        { demo::TAB_INPUT, "gamepad", "demo/tabs/input", "demo/home/link_input" },
        { demo::TAB_DEVELOPERS, "code", "demo/tabs/developers", "demo/home/link_developers" },
        { demo::TAB_ABOUT, "info", "demo/tabs/about", "demo/home/link_about" },
    };
    for (int line = 0; line < 2; line++)
    {
        aurore::Box* tiles = show::row();
        if (line == 1)
            tiles->setMarginTop(show::GAP);
        for (int i = 0; i < 4; i++)
        {
            const Link& l = links[line * 4 + i];
            tiles->addView(link(this, l.tab, l.icon, aurore::getStr(l.name), aurore::getStr(l.line)));
        }
        explore->addView(tiles);
    }
}

aurore::View* HomeTab::create()
{
    return new HomeTab();
}

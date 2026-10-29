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

#include "tab/about_tab.hpp"

#include <aurore/cadran.hpp>

#include "view/aurora_view.hpp"
#include "view/showcase.hpp"

using namespace aurore::literals; // for _i18n

namespace kit = cadran::kit;

namespace
{
// Who did what: a name, and a line under it
aurore::Box* credit(const char* icon, const std::string& name, const std::string& what)
{
    aurore::Box* card = show::card(true, true);
    card->setPadding(16, 18, 16, 18);
    aurore::Box* head = show::row(false, 10);
    head->setAlignItems(aurore::AlignItems::CENTER);
    head->addView(kit::icon(icon, 20, "cadran/teal"));
    aurore::Label* label = kit::label(name, kit::TEXT_BODY + 1, "cadran/text");
    label->setSingleLine(true);
    label->setShrinkToFit(true);
    label->setGrow(1);
    label->setWidth(0);
    head->addView(label);
    card->addView(head);
    card->addView(show::note(what));
    return card;
}

// A licence: what it covers, and its name in a pill
aurore::Box* licence(const std::string& what, const std::string& name)
{
    aurore::Box* line = show::row(false, 16);
    line->setAlignItems(aurore::AlignItems::CENTER);
    line->setMinHeight(44);
    aurore::Label* label = kit::label(what, kit::TEXT_BODY, "cadran/text");
    label->setGrow(1);
    label->setWidth(0);
    line->addView(label);
    aurore::Box* pill = kit::pill(name, "cadran/text3");
    pill->setShrink(0);
    line->addView(pill);
    return line;
}
} // namespace

AboutTab::AboutTab()
{
    aurore::Box* page = show::page(this);

    // The mark, a theme may replace it (image slot demo/about/art)
    auto* night = new AuroraView();
    night->setRidges(false);
    // Beside its words, or above them on a compact screen
    const bool compact = show::compact();
    night->setAxis(compact ? aurore::Axis::COLUMN : aurore::Axis::ROW);
    night->setAlignItems(compact ? aurore::AlignItems::FLEX_START : aurore::AlignItems::CENTER);
    night->setPadding(aurore::dp(28), aurore::dp(32), aurore::dp(28), aurore::dp(32));
    night->setGap(aurore::dp(compact ? 16 : 28));
    auto* art = new aurore::Image();
    art->setThemeImage("demo/about/art");
    art->setWidth(aurore::dp(compact ? 96 : 132));
    art->setHeight(aurore::dp(compact ? 96 : 132));
    art->setShrink(0);
    night->addView(art);
    aurore::Box* words = show::column(8);
    if (compact)
        words->setWidthPercentage(100);
    else
    {
        words->setGrow(1);
        words->setWidth(0);
    }
    aurore::Label* name = kit::display("demo/about/title"_i18n, 40);
    name->setTextColor(AuroraView::SNOW);
    words->addView(name);
    aurore::Label* description = kit::label("demo/about/description"_i18n, kit::TEXT_LEAD);
    description->setTextColor(AuroraView::SNOW);
    description->setLineHeight(1.4f);
    words->addView(description);
    aurore::Label* site = kit::label("aurore.boreale.dev  ·  github.com/JigSawFr/Aurore", kit::TEXT_NOTE);
    site->setTextColor(AuroraView::MIST);
    site->setMarginTop(6);
    words->addView(site);
    night->addView(words);
    page->addView(night);

    aurore::Box* links = show::row(true, 12);
    links->setMarginTop(show::gap());
    links->addView(show::iconButton("phone", "demo/about/qr"_i18n, []
        { ui::qr_dialog("demo/cadran/qr_text"_i18n, "https://aurore.boreale.dev/"); }));
    links->addView(show::iconButton("globe", "demo/about/site"_i18n, []
        { aurore::Application::getPlatform()->openBrowser("https://aurore.boreale.dev/"); }));
    page->addView(links);

    // The people it comes from
    aurore::Box* people = show::section(page, "heart", "demo/about/credits"_i18n, "demo/about/credits_lead"_i18n);
    aurore::Box* row    = show::row();
    row->addView(credit("sparkle", "natinusala", "demo/about/natinusala"_i18n));
    row->addView(credit("refresh", "xfangfang", "demo/about/xfangfang"_i18n));
    row->addView(credit("list", "dragonflylee", "demo/about/dragonflylee"_i18n));
    people->addView(row);
    aurore::Box* more = show::row();
    more->setMarginTop(show::gap());
    more->addView(credit("phone", "XITRIX", "demo/about/xitrix"_i18n));
    more->addView(credit("user", "demo/about/contributors"_i18n, "demo/about/contributors_text"_i18n));
    more->addView(credit("code", "JigSawFr", "demo/about/jigsawfr"_i18n));
    people->addView(more);

    // Licences
    aurore::Box* licences = show::section(page, "file", "demo/about/licences"_i18n, "demo/about/licences_lead"_i18n);
    aurore::Box* card     = show::card(false, false);
    card->setPadding(10, 22, 10, 22);
    card->addView(licence("demo/about/licence_core"_i18n, "Apache 2.0"));
    card->addView(licence("demo/about/licence_cadran"_i18n, "GPLv3"));
    card->addView(licence("nanovg · Yoga · {fmt} · TinyXML-2 · JSON for Modern C++ · Tweeny · stb", "zlib · MIT"));
    card->addView(licence("GLFW · SDL · libromfs · QR Code generator", "zlib · MIT"));
    card->addView(licence("Inter · PromptFont · Sora", "OFL 1.1"));
    card->addView(licence("Material Icons", "Apache 2.0"));
    card->addView(licence("Twemoji", "CC-BY 4.0"));
    licences->addView(card);
    aurore::Label* notices = show::note("demo/about/notices"_i18n);
    notices->setMarginTop(12);
    licences->addView(notices);
}

aurore::View* AboutTab::create()
{
    return new AboutTab();
}

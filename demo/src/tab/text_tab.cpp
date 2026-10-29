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

#include "tab/text_tab.hpp"

#include <aurore/cadran.hpp>

#include "demo_app.hpp"
#include "tab/text_test_tab.hpp"
#include "view/showcase.hpp"

using namespace aurore::literals; // for _i18n

namespace kit = cadran::kit;

namespace
{
// Each language's flag (Twemoji, resources/img/emoji), as Cadran's picker shows them
const char* const FLAGS[] = { "1f1fa-1f1f8", "1f1eb-1f1f7", "1f1e8-1f1e6", "1f1e9-1f1ea", "1f1ea-1f1f8", "1f1f2-1f1fd",
    "1f1ee-1f1f9", "1f1f3-1f1f1", "1f1f5-1f1f9", "1f1e7-1f1f7", "1f1f7-1f1fa", "1f1ef-1f1f5", "1f1f0-1f1f7",
    "1f1e8-1f1f3", "1f1f9-1f1fc" };

// A language's own name, its place shorter when it is spoken in two
std::string languageName(const std::string& code)
{
    static const std::pair<const char*, const char*> SHORT[] = {
        { "fr", "Français (FR)" },
        { "fr-CA", "Français (CA)" },
        { "es", "Español (ES)" },
        { "es-419", "Español (LatAm)" },
        { "pt", "Português (PT)" },
        { "pt-BR", "Português (BR)" },
    };
    for (const auto& [language, name] : SHORT)
        if (code == language)
            return name;
    return aurore::getStr("cadran/tools/languages/" + code);
}

// A language's tile: its flag and its own name; the one in use outlined
aurore::Box* language(int index, const std::string& code)
{
    aurore::Box* tile = kit::card(true, false);
    tile->setAxis(aurore::Axis::ROW);
    tile->setAlignItems(aurore::AlignItems::CENTER);
    tile->setPadding(12, 16, 12, 14);
    tile->setGap(12);
    tile->setHeight(60);
    tile->setWidth(214); // five to a row
    show::share(tile, 2);
    auto* flag = new aurore::Image();
    flag->setImageFromRes(std::string("img/emoji/") + FLAGS[index] + ".png");
    flag->setWidth(28);
    flag->setHeight(28);
    flag->setShrink(0);
    tile->addView(flag);
    aurore::Label* name = kit::label(languageName(code), kit::TEXT_BODY, "cadran/text");
    name->setGrow(1);
    name->setWidth(0);
    name->setShrinkToFit(true);
    name->setShrinkToFitMin(12);
    name->setSingleLine(true);
    tile->addView(name);
    show::markChoice(tile, code == live_prefs::locale());
    tile->registerClickAction([code](aurore::View*)
        {
        ui::prefs().language = code;
        ui::save_prefs();
        live_prefs::language_changed();
        return true; });
    tile->addGestureRecognizer(new aurore::TapGestureRecognizer(tile));
    return tile;
}

aurore::Label* sample(const std::string& text, float size, bool display = false)
{
    aurore::Label* label = display ? kit::display(text, size) : kit::label(text, size, "cadran/text");
    label->setSingleLine(true);
    return label;
}
} // namespace

TextTab::TextTab()
{
    aurore::Box* page = show::page(this);

    // 15 languages, switched while the app runs
    aurore::Box* languages = show::section(page, "language", "demo/text/languages"_i18n, "demo/text/languages_lead"_i18n);
    aurore::Box* tiles     = show::row(true, 12);
    for (size_t i = 1; i < std::size(ui::LANGUAGES); i++)
        tiles->addView(language((int)i - 1, ui::LANGUAGES[i]));
    languages->addView(tiles);

    // Sizes
    aurore::Box* sizes                                = show::section(page, "text-large", "demo/text/sizes"_i18n, "demo/text/sizes_lead"_i18n);
    aurore::Box* row                                  = show::row(true, 12);
    const std::pair<const char*, const char*> SIZES[] = { { "auto", "tv" }, { "normal", "text-normal" },
        { "large", "text-large" }, { "largest", "text-largest" } };
    for (const auto& [size, icon] : SIZES)
    {
        std::string chosen = size;
        aurore::Box* tile  = show::choice(icon, aurore::getStr(std::string("cadran/prefs/text_sizes/") + size), [this, chosen]
             {
            ui::prefs().text_size = chosen;
            ui::save_prefs();
            text_size::apply();
            for (auto& [tile, name] : this->sizeChoices)
                show::markChoice(tile, name == ui::prefs().text_size); });
        show::markChoice(tile, ui::prefs().text_size == size);
        this->sizeChoices.emplace_back(tile, size);
        row->addView(tile);
    }
    sizes->addView(row);
    aurore::Box* scale = show::card(false, false);
    scale->setMarginTop(show::gap());
    scale->setGap(10);
    scale->addView(sample("Aurore 2026", 40, true));
    scale->addView(sample("demo/text/scale_title"_i18n, kit::TEXT_TITLE));
    scale->addView(sample("demo/text/scale_lead"_i18n, kit::TEXT_LEAD));
    scale->addView(sample("demo/text/scale_body"_i18n, kit::TEXT_BODY));
    aurore::Label* meta = sample("demo/text/scale_meta"_i18n, kit::TEXT_META);
    meta->setThemeColor("textColor", "cadran/text2");
    scale->addView(meta);
    sizes->addView(scale);

    // Scripts and emoji
    aurore::Box* scripts = show::section(page, "globe", "demo/scripts/scripts"_i18n, "demo/text/scripts_lead"_i18n);
    aurore::Box* card    = show::card(true, false);
    card->setGap(12);
    aurore::Label* hello = kit::label("Bonjour · Hallo · Hola · Ciao · Olá · Привет · こんにちは · 안녕하세요 · 你好 · 您好", 24,
        "cadran/text");
    card->addView(hello);
    aurore::Box* emoji = show::row(true, 14);
    for (const char* code : { "1f680", "1f3ae", "2705", "2b50", "1f389", "1f30d", "2764", "2728", "1f319", "1f3a8",
             "1f4a1", "1f514" })
    {
        auto* image = new aurore::Image();
        image->setImageFromRes(std::string("img/emoji/") + code + ".png");
        image->setWidth(36);
        image->setHeight(36);
        emoji->addView(image);
    }
    card->addView(emoji);
    card->addView(kit::label("demo/text/emoji_note"_i18n, kit::TEXT_NOTE, "cadran/text2"));
    card->addView(kit::label("Ça déménage à Zürich, ñandú, łódź · Καλημέρα κόσμε", kit::TEXT_BODY, "cadran/text2"));
    scripts->addView(card);

    // Long texts
    aurore::Box* longText = show::section(page, "lines", "demo/text/long"_i18n, "demo/text/long_lead"_i18n);
    aurore::Box* longRow  = show::row();
    aurore::Box* marquee  = show::card(true, true);
    marquee->addView(show::note("demo/text/marquee"_i18n));
    aurore::Label* scrolling = kit::label("demo/text/marquee_text"_i18n, kit::TEXT_LEAD, "cadran/text");
    scrolling->setSingleLine(true);
    scrolling->setMarginTop(8);
    marquee->addView(scrolling);
    marquee->registerClickAction([](aurore::View*)
        { return true; });
    longRow->addView(marquee);
    aurore::Box* fit = show::card(true, true);
    fit->addView(show::note("demo/text/shrink"_i18n));
    aurore::Label* shrunk = kit::label("demo/text/shrink_text"_i18n, kit::TEXT_LEAD, "cadran/text");
    shrunk->setSingleLine(true);
    shrunk->setShrinkToFit(true);
    shrunk->setShrinkToFitMin(11);
    shrunk->setMarginTop(8);
    fit->addView(shrunk);
    longRow->addView(fit);
    longText->addView(longRow);
    aurore::Box* paragraph = show::card(true, false);
    paragraph->setMarginTop(show::gap());
    aurore::Label* body = kit::label("demo/text/paragraph"_i18n, kit::TEXT_BODY, "cadran/text2");
    body->setLineHeight(1.5f);
    paragraph->addView(body);
    longText->addView(paragraph);

    // The text lab
    aurore::Box* lab  = show::section(page, "pencil", "demo/text/lab"_i18n);
    aurore::Box* rows = cadran::List::rows_in(lab);
    auto* open        = new aurore::DetailCell();
    open->setText("demo/text/lab_open"_i18n);
    open->setDetailText(show::compact() ? "›" : "demo/text/lab_detail"_i18n + "  ›");
    open->registerClickAction([](aurore::View*)
        {
        demo::pushScreen(TextTestTab::create(), "demo/text/lab_open"_i18n);
        return true; });
    cadran::List::row(open, "pencil");
    rows->addView(open);
}

aurore::View* TextTab::create()
{
    return new TextTab();
}

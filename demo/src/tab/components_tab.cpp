/*
    Copyright 2021 natinusala
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

#include "tab/components_tab.hpp"

#include <aurore/cadran.hpp>

#include "demo_app.hpp"
#include "tab/cadran_tab.hpp"
#include "tab/settings_tab.hpp"
#include "view/showcase.hpp"

using namespace aurore::literals; // for _i18n

namespace kit = cadran::kit;

namespace
{
// "Option 1" to "Option <count>"
std::vector<std::string> options(int count)
{
    std::vector<std::string> names;
    for (int i = 1; i <= count; i++)
        names.push_back(aurore::getStr("demo/components_more/option", i));
    return names;
}

// A borealis button in one of its styles, as XML would make it
aurore::Button* styled(aurore::ButtonStyle const* look, const std::string& text)
{
    auto* button = new aurore::Button();
    button->setStyle(look);
    button->setText(text);
    button->setHeight(48);
    button->setPadding(0, 22, 0, 22);
    return button;
}

// What a scaling type does to the same picture, and its name under it
aurore::Box* scaled(aurore::ImageScalingType type, const std::string& name)
{
    aurore::Box* card = show::card(true, true);
    card->setPadding(12, 12, 14, 12);
    card->setAlignItems(aurore::AlignItems::STRETCH);
    auto* image = new aurore::Image();
    image->setImageFromRes("img/tiles.png");
    image->setScalingType(type);
    image->setHeight(96);
    image->setCornerRadius(10);
    image->setThemeColor("backgroundColor", "cadran/surf2");
    card->addView(image);
    aurore::Label* label = kit::label(name, kit::TEXT_NOTE, "cadran/text2");
    label->setMarginTop(10);
    card->addView(show::centred(label));
    card->setAccessibilityLabel(name);
    return card;
}
} // namespace

ComponentsTab::ComponentsTab()
{
    aurore::Box* page = show::page(this);

    // Buttons
    aurore::Box* buttons = show::section(page, "sparkle", "demo/components/buttons_header"_i18n, "demo/components/buttons_lead"_i18n);
    aurore::Box* card    = show::card(false, false);
    aurore::Label* line  = show::note("demo/components/borealis_styles"_i18n);
    line->setMarginTop(0);
    line->setMarginBottom(12);
    card->addView(line);
    aurore::Box* row = show::row(true, 12);
    row->addView(styled(&aurore::BUTTONSTYLE_PRIMARY, "demo/components/button_primary"_i18n));
    row->addView(styled(&aurore::BUTTONSTYLE_HIGHLIGHT, "demo/components/button_highlight"_i18n));
    row->addView(styled(&aurore::BUTTONSTYLE_DEFAULT, "demo/components/button_default"_i18n));
    row->addView(styled(&aurore::BUTTONSTYLE_BORDERED, "demo/components/button_bordered"_i18n));
    row->addView(styled(&aurore::BUTTONSTYLE_BORDERLESS, "demo/components/button_borderless"_i18n));
    card->addView(row);

    line = show::note("demo/components/cadran_styles"_i18n);
    line->setMarginTop(22);
    line->setMarginBottom(12);
    card->addView(line);
    row = show::row(true, 12);
    row->setAlignItems(aurore::AlignItems::CENTER);
    row->addView(show::button("demo/components/button_primary"_i18n, [] {}, true));
    row->addView(show::button("demo/components/button_plain"_i18n, [] {}));
    aurore::Button* danger = show::button("demo/components/button_danger"_i18n, [] {});
    cadran::style_button(danger, cadran::Emphasis::DANGER);
    row->addView(danger);
    aurore::Button* disabled = show::button("demo/components/button_disabled"_i18n, [] {});
    disabled->setState(aurore::ButtonState::DISABLED);
    disabled->setStateStyle(aurore::StyleState::DISABLED, "alpha", "0.45");
    row->addView(disabled);
    row->addView(show::iconButton("download", "demo/components/icon_text"_i18n, []
        { ui::notify("demo/components/icon_text"_i18n); }));
    auto* more = new aurore::IconButton();
    more->setId("demo/more");
    more->setGlyph("\uE5D4"); // Material Icons' "more"
    more->setAccessibilityLabel("demo/components/icon_menu"_i18n);
    more->registerClickAction([](aurore::View* view)
        {
        auto* menu  = new aurore::ContextMenu(view);
        auto chosen = [](const std::string& what) { return [what]() { ui::notify(aurore::getStr("demo/components/menu_chosen", what)); }; };
        menu->setTitle("demo/components/menu_title"_i18n);
        menu->addItem("demo/components/menu_open"_i18n, chosen("demo/components/menu_open"_i18n), kit::icon("eye", 22));
        menu->addItem("demo/components/menu_favourite"_i18n, chosen("demo/components/menu_favourite"_i18n), kit::icon("heart", 22));
        menu->addItem("demo/components/menu_share"_i18n, chosen("demo/components/menu_share"_i18n), kit::icon("upload", 22));
        menu->addSeparator();
        menu->addItem("demo/components/menu_delete"_i18n, chosen("demo/components/menu_delete"_i18n), kit::icon("trash", 22, "cadran/coral"));
        menu->open();
        return true; });
    row->addView(more);
    card->addView(row);
    buttons->addView(card);

    // Toggles and values: borealis' cells, as rows of a Cadran card
    aurore::Box* values = show::section(page, "settings", "demo/components/values"_i18n, "demo/components/values_lead"_i18n);
    aurore::Box* rows   = cadran::List::rows_in(values);
    auto* toggle        = new aurore::BooleanCell();
    toggle->init("demo/components/toggle"_i18n, true, [](bool) {});
    cadran::List::row(toggle, "bolt");
    rows->addView(toggle);
    auto* detail = new aurore::DetailCell();
    detail->setText("demo/components/detail"_i18n);
    detail->setDetailText("demo/components/detail_value"_i18n + "  ›");
    detail->registerClickAction([](aurore::View*)
        {
        ui::info("demo/components/detail_info"_i18n);
        return true; });
    cadran::List::row(detail, "info");
    rows->addView(detail);
    auto* selector = new aurore::SelectorCell();
    selector->init("demo/components/selector"_i18n, options(13), 0, [](int) {});
    cadran::List::row(selector, "list");
    rows->addView(selector);
    auto* input = new aurore::InputCell();
    input->init("demo/components/input"_i18n, "Aurore", [](std::string) {}, "demo/components/input_placeholder"_i18n, "demo/components/input_hint"_i18n);
    cadran::List::row(input, "pencil");
    rows->addView(input);
    auto* number = new aurore::InputNumericCell();
    number->init("demo/components/number"_i18n, 2448, [](long) {}, "demo/components/input_hint"_i18n);
    cadran::List::row(number, "key");
    rows->addView(number);
    auto* slider = new aurore::SliderCell();
    slider->init("demo/components/volume"_i18n, 0.6f, [slider](float value)
        { slider->setDetailText(std::to_string((int)(value * 100 + 0.5f)) + " %"); });
    slider->setDetailText("60 %");
    cadran::List::row(slider, "bell");
    rows->addView(slider);

    aurore::Box* settings = show::row(true, 20);
    settings->setMarginTop(show::gap());
    settings->setAlignItems(aurore::AlignItems::CENTER);
    settings->addView(show::iconButton("settings", "demo/components/open_settings"_i18n, []
        { demo::pushScreen(SettingsTab::create(), "demo/components/settings_title"_i18n, false); }));
    aurore::Label* settingsNote = show::note("demo/components/open_settings_note"_i18n);
    settingsNote->setMarginTop(0);
    settingsNote->setGrow(1);
    settingsNote->setWidth(0);
    settings->addView(settingsNote);
    values->addView(settings);

    // Progress
    aurore::Box* progress = show::section(page, "download", "demo/components/progress"_i18n, "demo/components/progress_lead"_i18n);
    aurore::Box* bars     = show::row();
    aurore::Box* barCard  = show::card();
    barCard->addView(show::title("demo/components/bar"_i18n));
    auto* bar = new kit::Bar();
    bar->set(0.62f);
    bar->setMarginTop(16);
    barCard->addView(bar);
    auto* loading = new LoadingBar();
    loading->setHeight(8);
    loading->setMarginTop(20);
    loading->setProgress(0.35f);
    barCard->addView(loading);
    bars->addView(barCard);
    aurore::Box* sliderCard = show::card();
    sliderCard->addView(show::title("demo/components/slider"_i18n));
    auto* bare = new aurore::Slider();
    bare->setProgress(0.4f);
    bare->setHeight(40);
    bare->setMarginTop(8);
    sliderCard->addView(bare);
    bars->addView(sliderCard);
    progress->addView(bars);

    // Chips, pills and keys
    aurore::Box* chips    = show::section(page, "tag", "demo/components/chips"_i18n, "demo/components/chips_lead"_i18n);
    aurore::Box* chipCard = show::card(false, false);
    aurore::Box* chipRow  = show::row(true, 12);
    for (const char* text : { "15 min", "30 min", "1 h", "2 h" })
        chipRow->addView(kit::chip(text));
    chipCard->addView(chipRow);
    aurore::Box* pills = show::row(true, 10);
    pills->setMarginTop(18);
    pills->addView(kit::pill("demo/components/pill_ok"_i18n, "cadran/teal"));
    pills->addView(kit::pill("demo/components/pill_warn"_i18n, "cadran/amber"));
    pills->addView(kit::pill("demo/components/pill_bad"_i18n, "cadran/coral"));
    pills->addView(kit::pill("demo/components/pill_info"_i18n, "cadran/text3"));
    chipCard->addView(pills);
    aurore::Box* keys = show::row(true, 10);
    keys->setMarginTop(18);
    keys->setAlignItems(aurore::AlignItems::CENTER);
    for (const char* key : { "A", "B", "X", "Y", "L", "R" })
        keys->addView(kit::key(key, 30));
    chipCard->addView(keys);
    chips->addView(chipCard);

    // Dialogs and sheets
    aurore::Box* dialogs    = show::section(page, "lines", "demo/cadran/dialogs"_i18n, "demo/components/dialogs_lead"_i18n);
    aurore::Box* dialogCard = show::card(false, false);
    dialogCard->addView(cadran_pieces::dialogs());
    dialogs->addView(dialogCard);

    // Toasts and notifications
    aurore::Box* toasts    = show::section(page, "bell", "demo/components/toasts"_i18n, "demo/components/toasts_lead"_i18n);
    aurore::Box* toastCard = show::card(false, false);
    toastCard->addView(cadran_pieces::toasts());
    toasts->addView(toastCard);

    // Images
    aurore::Box* images   = show::section(page, "eye", "demo/components/images_header_title"_i18n, "demo/components/images_lead"_i18n);
    aurore::Box* imageRow = show::row();
    imageRow->addView(scaled(aurore::ImageScalingType::FIT, "demo/components/images_fit"_i18n));
    imageRow->addView(scaled(aurore::ImageScalingType::FILL, "demo/components/images_fill"_i18n));
    imageRow->addView(scaled(aurore::ImageScalingType::STRETCH, "demo/components/images_stretched"_i18n));
    imageRow->addView(scaled(aurore::ImageScalingType::CENTER, "demo/components/images_cropped"_i18n));
    images->addView(imageRow);

    // Icons
    aurore::Box* icons    = show::section(page, "eye", "demo/cadran/icons"_i18n, "demo/components/icons_lead"_i18n);
    aurore::Box* iconCard = show::card(false, false);
    iconCard->setPadding(12, 14, 12, 14);
    iconCard->addView(cadran_pieces::icons());
    icons->addView(iconCard);

    // An empty list, and Markdown
    aurore::Box* text    = show::section(page, "file", "demo/components/empty_markdown"_i18n);
    aurore::Box* textRow = show::row();
    textRow->setAlignItems(aurore::AlignItems::STRETCH);
    kit::Empty empty = kit::empty("shelf");
    empty.title->setText("demo/components/empty_title"_i18n);
    empty.text->setText("demo/components/empty_text"_i18n);
    empty.box->setGrow(1);
    empty.box->setWidth(0);
    empty.box->setJustifyContent(aurore::JustifyContent::CENTER);
    textRow->addView(empty.box);
    aurore::Box* markdown = show::card();
    cadran_pieces::markdown(markdown);
    textRow->addView(markdown);
    text->addView(textRow);
}

aurore::View* ComponentsTab::create()
{
    return new ComponentsTab();
}

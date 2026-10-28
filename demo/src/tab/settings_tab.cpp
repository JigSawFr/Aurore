/*
    Copyright 2021 natinusala

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

#include "tab/settings_tab.hpp"

#include <aurore/cadran/ui/live_prefs.hpp>
#include <aurore/cadran/ui/prefs.hpp>
#include <aurore/cadran/ui/ui.hpp>

using namespace aurore::literals; // for _i18n

bool radioSelected = false;

// "Option 1" to "Option <count>"
static std::vector<std::string> options(int count)
{
    std::vector<std::string> names;
    for (int i = 1; i <= count; i++)
        names.push_back(aurore::getStr("demo/components_more/option", i));
    return names;
}

static std::vector<std::string> NOTIFICATIONS = {
    "You have cool hair",
    "I like your shoes",
    "borealis is powered by nanovg",
    "The Triforce is an inside job",
    "Pozznx will trigger in one day and twelve hours",
    "Aurora Borealis? At this time of day, at this time of year, in this part of the gaming market, located entirely within your Switch?!",
    "May I see it?",
    "Hmm, Steamed Hams!",
    "Hello\nWorld!"
};

SettingsTab::SettingsTab()
{
    // Inflate the tab from the XML file
    this->inflateFromXMLRes("xml/tabs/settings.xml");

    radio->title->setText("demo/settings/radio"_i18n);
    radio->setSelected(radioSelected);
    radio->registerClickAction([this](aurore::View* view)
        {
        radioSelected = !radioSelected;
        this->radio->setSelected(radioSelected);
        return true; });

    boolean->title->setText("demo/settings/switch"_i18n);

    debug->init("demo/settings/debug"_i18n, aurore::Application::isDebuggingViewEnabled(), [](bool value)
        {
        aurore::Application::enableDebuggingView(value);
        aurore::sync([value](){
            aurore::Logger::info("{} the debug layer", value ? "Open" : "Close");
        }); });

    screenSaver->init("demo/settings/screensaver"_i18n, aurore::Application::getPlatform()->isScreenDimmingDisabled(), [](bool value)
        { aurore::Application::getPlatform()->disableScreenDimming(value); });

    bottomBar->init("demo/settings/bottom_bar"_i18n, !aurore::AppletFrame::HIDE_BOTTOM_BAR, [](bool value)
        {
        aurore::AppletFrame::HIDE_BOTTOM_BAR = !value;
        auto stack = aurore::Application::getActivitiesStack();
        for (auto& activity : stack) {
            auto* frame = dynamic_cast<aurore::AppletFrame*>(
                activity->getContentView());
            if (!frame) continue;
            frame->setFooterVisibility(!value ? aurore::Visibility::GONE
                                             : aurore::Visibility::VISIBLE);
        } });

    fps->init("demo/settings/fps"_i18n, aurore::Application::getFPSStatus(), [](bool value)
        { aurore::Application::setFPSStatus(value); });

    swapInterval->init("demo/settings/swap_interval"_i18n, { "0", "1", "2", "3", "4" }, 1, [](int selected) {}, [](int selected)
        { aurore::Application::setSwapInterval(selected); });

    alwaysOnTop->init("demo/settings/always_on_top"_i18n, false, [](bool value)
        { aurore::Application::getPlatform()->setWindowAlwaysOnTop(value); });

    selector->init("demo/settings/selector"_i18n, options(13), 0, [](int selected) {}, [](int selected)
        {
        auto dialog = new aurore::Dialog(aurore::getStr("demo/settings/selected", selected + 1));
        dialog->addButton("hints/ok"_i18n, []() {});
        dialog->open(); });

    input->init(
        "demo/settings/input"_i18n, "https://github.com", [](std::string text) {

        },
        "demo/settings/input_placeholder"_i18n, "demo/settings/input_hint"_i18n);

    inputNumeric->init(
        "demo/settings/input_number"_i18n, 2448, [](int number) {

        },
        "demo/settings/input_hint"_i18n);

    ipAddress->setDetailText(aurore::Application::getPlatform()->getIpAddress());
    dnsServer->setDetailText(aurore::Application::getPlatform()->getDnsServer());

    input->registerAction("hints/open"_i18n, aurore::BUTTON_X, [](aurore::View* view)
        {
        aurore::DetailCell *cell = dynamic_cast<aurore::DetailCell *>(view);
        aurore::Application::getPlatform()->openBrowser(cell->detail->getFullText());
        return true; }, false, false, aurore::SOUND_CLICK);

    float brightness = aurore::Application::getPlatform()->getBacklightBrightness();
    slider->init("demo/settings/brightness"_i18n, brightness, [this](float value)
        {
        aurore::Application::getPlatform()->setBacklightBrightness(value);
        slider->setDetailText(fmt::format("{:.2f}", value)); });
    slider->setDetailText(fmt::format("{:.2f}", brightness));

    notify->registerClickAction([](...)
        {
        std::string notification = NOTIFICATIONS[std::rand() % NOTIFICATIONS.size()];
        aurore::Application::notify(notification);
        return true; });

    // The variant and the theme file go through Cadran's preferences, as
    // its own pickers do (the header's theme button): the screens are drawn
    // in them from the next frame, nothing is built again.
    themeDark->init("demo/settings/dark"_i18n, ui::prefs().theme != "light", [](bool dark)
        {
        ui::prefs().theme = dark ? "dark" : "light";
        live_prefs::apply_theme(); });
    this->listThemes();
}

void SettingsTab::listThemes()
{
    std::vector<aurore::ThemeInfo> files = aurore::Theme::builtinThemes();
    for (aurore::ThemeInfo& info : aurore::Theme::listThemes("@res/themes"))
        files.push_back(std::move(info));

    aurore::ThemeInfo none;
    none.name  = "demo/settings/default_theme"_i18n;
    none.light = none.dark = true;
    files.insert(files.begin(), none);

    for (const aurore::ThemeInfo& info : files)
    {
        auto* cell = new aurore::DetailCell();
        cell->setText(info.author.empty() ? info.name : info.name + "  ·  " + info.author);

        // Its colours, as it draws the variant it shows first
        if (!info.path.empty())
        {
            const aurore::ThemeInfo::Preview& preview = info.dark ? info.darkPreview : info.lightPreview;
            auto* swatches                            = new aurore::Box(aurore::Axis::ROW);
            swatches->setMarginRight(16);
            for (NVGcolor color : { preview.background, preview.surface, preview.text, preview.accent, preview.highlight })
            {
                auto* swatch = new aurore::Rectangle(color);
                swatch->setSize(aurore::Size(22, 22));
                swatch->setCornerRadius(11);
                swatch->setMarginLeft(6);
                swatch->setBorderThickness(1);
                swatch->setThemeColor("borderColor", "aurore/header/border");
                swatches->addView(swatch);
            }
            cell->addView(swatches, (int)cell->getChildren().size() - 1);
        }

        if (!info.ok())
            cell->setDetailText(aurore::getStr("demo/settings/errors", info.errors.size(), info.errors.front()));
        else if (!info.warnings.empty())
            cell->setDetailText(aurore::getStr("demo/settings/warnings", info.warnings.size()));

        std::string path = info.path;
        cell->registerClickAction([this, path](aurore::View*)
            {
            ui::prefs().theme_file = path;
            live_prefs::apply_theme();
            for (auto& [line, file] : this->themeCells)
                line->setDetailText(file == path ? "demo/settings/active"_i18n : "");
            this->themeDark->setOn(aurore::Application::getThemeVariant() == aurore::ThemeVariant::DARK, false);
            return true; });
        if (info.path == ui::prefs().theme_file)
            cell->setDetailText("demo/settings/active"_i18n);
        this->themeCells.emplace_back(cell, path);
        this->themes->addView(cell);
    }
}

aurore::View* SettingsTab::create()
{
    return new SettingsTab();
}

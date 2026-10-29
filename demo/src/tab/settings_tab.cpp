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

using namespace aurore::literals; // for _i18n

// "Option 1" to "Option <count>"
static std::vector<std::string> options(int count)
{
    std::vector<std::string> names;
    for (int i = 1; i <= count; i++)
        names.push_back(aurore::getStr("demo/components_more/option", i));
    return names;
}

SettingsTab::SettingsTab()
{
    // Inflate the tab from the XML file
    this->inflateFromXMLRes("xml/tabs/settings.xml");

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
        aurore::Application::notify("demo/cadran/notification_text"_i18n);
        return true; });
}

aurore::View* SettingsTab::create()
{
    return new SettingsTab();
}

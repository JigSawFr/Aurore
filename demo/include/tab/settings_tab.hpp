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

#pragma once

#include <aurore.hpp>

class SettingsTab : public aurore::Box
{
  public:
    SettingsTab();

    BRLS_BIND(aurore::RadioCell, radio, "radio");
    BRLS_BIND(aurore::BooleanCell, boolean, "boolean");
    BRLS_BIND(aurore::SelectorCell, selector, "selector");
    BRLS_BIND(aurore::InputCell, input, "input");
    BRLS_BIND(aurore::InputNumericCell, inputNumeric, "inputNumeric");
    BRLS_BIND(aurore::DetailCell, ipAddress, "ipAddress");
    BRLS_BIND(aurore::DetailCell, dnsServer, "dnsServer");
    BRLS_BIND(aurore::BooleanCell, debug, "debug");
    BRLS_BIND(aurore::BooleanCell, screenSaver, "screenSaver");
    BRLS_BIND(aurore::BooleanCell, bottomBar, "bottomBar");
    BRLS_BIND(aurore::BooleanCell, alwaysOnTop, "alwaysOnTop");
    BRLS_BIND(aurore::BooleanCell, fps, "fps");
    BRLS_BIND(aurore::SelectorCell, swapInterval, "swapInterval");
    BRLS_BIND(aurore::SliderCell, slider, "slider");
    BRLS_BIND(aurore::DetailCell, notify, "notify");
    BRLS_BIND(aurore::BooleanCell, themeDark, "themeDark");
    BRLS_BIND(aurore::Box, themes, "themes");

    static aurore::View* create();

  private:
    // A line per theme file (Theme::builtinThemes, Theme::listThemes): its
    // name, a few of its colours, what is wrong with it; A uses it.
    void listThemes();
    std::vector<std::pair<aurore::DetailCell*, std::string>> themeCells;
};

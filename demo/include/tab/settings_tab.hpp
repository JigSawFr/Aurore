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

/// borealis' settings cells, as XML lays them out (resources/xml/tabs/
/// settings.xml): the Components tab opens them on a screen of their own,
/// which Cadran dresses in cards.
class SettingsTab : public aurore::Box
{
  public:
    SettingsTab();

    AURORE_BIND(aurore::BooleanCell, boolean, "boolean");
    AURORE_BIND(aurore::SelectorCell, selector, "selector");
    AURORE_BIND(aurore::InputCell, input, "input");
    AURORE_BIND(aurore::InputNumericCell, inputNumeric, "inputNumeric");
    AURORE_BIND(aurore::DetailCell, ipAddress, "ipAddress");
    AURORE_BIND(aurore::DetailCell, dnsServer, "dnsServer");
    AURORE_BIND(aurore::BooleanCell, debug, "debug");
    AURORE_BIND(aurore::BooleanCell, screenSaver, "screenSaver");
    AURORE_BIND(aurore::BooleanCell, bottomBar, "bottomBar");
    AURORE_BIND(aurore::BooleanCell, alwaysOnTop, "alwaysOnTop");
    AURORE_BIND(aurore::BooleanCell, fps, "fps");
    AURORE_BIND(aurore::SelectorCell, swapInterval, "swapInterval");
    AURORE_BIND(aurore::SliderCell, slider, "slider");
    AURORE_BIND(aurore::DetailCell, notify, "notify");

    static aurore::View* create();
};

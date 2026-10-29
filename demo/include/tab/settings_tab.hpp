// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

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

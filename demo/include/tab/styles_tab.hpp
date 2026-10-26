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

#pragma once

#include <borealis.hpp>

// State styles, transitions, data binding and the focus announcer
// (docs/styles.md), from resources/xml/tabs/styles.xml.
class StylesTab : public brls::Box
{
  public:
    StylesTab();

    static brls::View* create();

  private:
    BRLS_BIND(brls::Button, disabled, "styles/disabled");
    BRLS_BIND(brls::Button, toggle, "styles/toggle");
    BRLS_BIND(brls::Button, increment, "styles/increment");
    BRLS_BIND(brls::BooleanCell, reduceMotion, "styles/reduce_motion");
};

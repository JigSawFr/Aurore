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

#include <cstdint>
#include <aurore.hpp>
#include <aurore/cadran/view/cadran_digits.hpp>
#include <aurore/cadran/view/scroll_view.hpp>

/// For developers: the frame measured live (aurore::Profiler), the
/// inspector, the debug layer and redraw on demand, the layout lab, data
/// binding and what a screen reader would say, how the pages are written.
class DevelopersTab : public ScrollView
{
  public:
    DevelopersTab();
    ~DevelopersTab() override;

    void willAppear(bool resetState) override;
    void willDisappear(bool resetState) override;

    static aurore::View* create();

  private:
    CadranDigits* fps;
    CadranDigits* cpu;
    CadranDigits* calls;
    CadranDigits* laidOut;

    bool measuring  = false;
    bool wasEnabled = false;
    aurore::Event<>::Subscription subscription;
    uint32_t frames = 0, drawCalls = 0, views = 0;
    aurore::Time interval = 0, work = 0;

    void frameMeasured();
    void stopMeasuring();
};

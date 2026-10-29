// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

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

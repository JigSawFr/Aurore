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

#include <vector>
#include <aurore.hpp>
#include <aurore/cadran/view/scroll_view.hpp>

class EasingPlot;

/// Movement: easings drawn and played, state styles with their
/// transitions (XML only), the focus highlight gliding and shaking, reduced
/// motion, and the transform lab.
class MotionTab : public ScrollView
{
  public:
    MotionTab();

    static aurore::View* create();

  private:
    std::vector<EasingPlot*> plots;
    aurore::Animatable progress;

    void play();
};

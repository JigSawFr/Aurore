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

#pragma once

#include <aurore.hpp>
#include <aurore/cadran/view/scroll_view.hpp>

/// Every control, grouped: buttons, toggles and values, progress, chips and
/// pills, dialogs and sheets, toasts, images, icons, Markdown, and a screen
/// of borealis' settings cells as Cadran dresses them.
class ComponentsTab : public ScrollView
{
  public:
    ComponentsTab();

    static aurore::View* create();
};

// SPDX-FileCopyrightText: 2020-2021 natinusala
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>

namespace aurore
{
class View;
}

// What the demo sets up around Aurore, shared by its main() and its tests
// (tests/): Cadran's hooks, before Application::init(), and the demo's views,
// colours and metrics, after Application::createWindow().
namespace demo
{
void installHooks();
void registerViews();

/// The rail's tabs, by their place in it (separators counted, as
/// aurore::TabFrame::focusTab and ui::go_to_tab count them).
enum Tab
{
    TAB_HOME       = 0,
    TAB_LIBRARY    = 1,
    TAB_COMPONENTS = 2,
    TAB_MOTION     = 3,
    TAB_TEXT       = 4,
    TAB_THEMES     = 5,
    TAB_INPUT      = 6,
    TAB_DEVELOPERS = 8,
    TAB_ABOUT      = 10,
    TAB_COUNT      = 11,
};
bool isSeparator(int position);
/// A tab's short name in English, for files and logs ("home", "library"…).
const char* tabName(int position);

/// Shows the tab at `position` of the main screen and moves the focus into
/// it, from a view inside the main screen (a quick link).
void openTab(aurore::View* from, int position);

/// `content` on a screen of its own over the one shown, titled `title`,
/// with a fade. Cadran gives it its header; `dressed`: the screen is laid
/// out in Cadran's pieces already, so its list is not dressed again in
/// cards (a screen of borealis cells is, when false).
void pushScreen(aurore::View* content, const std::string& title, bool dressed = true);
} // namespace demo

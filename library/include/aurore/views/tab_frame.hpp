// SPDX-FileCopyrightText: 2019-2021 natinusala
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <map>
#include <aurore/core/bind.hpp>
#include <aurore/views/applet_frame.hpp>
#include <aurore/views/sidebar.hpp>

namespace aurore
{

typedef std::function<View*(void)> TabViewCreator;

// An applet frame containing a sidebar on the left with multiple tabs which content is showing on the right.
// Only one tab is kept in memory at all times : when switching, the current tab is freed before the the new one is instantiated.
class TabFrame : public Box
{
  public:
    TabFrame();

    void handleXMLElement(tinyxml2::XMLElement* element) override;

    void addTab(std::string label, TabViewCreator creator);
    void focusTab(int position);
    void clearTabs();
    void addSeparator();

    /**
     * When on, a tab's content is kept once made rather than freed when
     * another tab is shown: coming back to it finds its scroll and focus as
     * left, without making it again. Off by default (one tab in memory).
     */
    void setKeepTabs(bool keep);

    ~TabFrame();

    static View* create();

  private:
    AURORE_BIND(Sidebar, sidebar, "aurore/tab_frame/sidebar");

    View* activeTab = nullptr;
    bool keepTabs   = false;
    int tabCount    = 0;
    std::map<int, View*> keptTabs;

    void freeKeptTabs();
};

} // namespace aurore

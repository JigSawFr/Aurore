// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-FileCopyrightText: 2026 Taylor
// SPDX-License-Identifier: GPL-3.0-or-later

// CadranTab — the common shape of a sidebar tab: it inflates its XML, re-reads
// its state each time it (re)appears, X refreshes it on demand (on the tabs
// that read the console; the footer shows only what a tab can do), a timer
// can refresh it while it is on screen, and the read-only note (a Label with
// the id "tab_ro_note", if the XML has one) shows while the app is read-only
// (ui::hooks::Hooks::read_only). An app derives its own tab from it and
// answers the hooks refresh_shown, shown_screen and main_screen with the
// statics below.
#pragma once

#include <functional>
#include <string>
#include <aurore.hpp>

#include <aurore/cadran/ui/guarded.hpp>

class CadranTab : public aurore::Box
{
  public:
    /// `refreshes`: X reads the tab again. Off for a tab that shows only the
    /// app's own settings and files, which change only from it.
    explicit CadranTab(const std::string& xml_res, bool refreshes = true);
    ~CadranTab() override;

    /// Called when the tab is shown and when the main screen comes back on top
    /// (after a screen, a dropdown, a dialog …).
    void willAppear(bool resetState = false) override;
    void onChildFocusGained(aurore::View* directChild, aurore::View* focusedView) override;

    /// Re-reads the tab on screen, if any (after the read-only mode changed).
    static void refresh_shown();
    /// The shown tab's screen ("tab/library"), "" for none.
    static std::string shown_screen();
    /// The main screen's tab last shown ("tab/about"): not a tab's view on a
    /// screen of its own. A sub-screen's way back.
    static std::string main_screen();

  protected:
    virtual void refresh() = 0;
    /// Each time the tab appears, before refresh() (an app's marks, laid out
    /// again).
    virtual void before_refresh() { }
    /// Each time the tab appears, after refresh().
    virtual void appeared() { }

    /// Goes with the tab: what finishes later (a flow's callback after the
    /// network or a dialog, a background task) checks it, by ui::guarded, as a
    /// tab replaced by another is freed meanwhile.
    ui::Alive alive = ui::make_alive();
    /// refresh(), once a flow is over: nothing if the tab is gone by then.
    std::function<void()> refresher()
    {
        return ui::guarded(this->alive, [this]()
            { this->refresh(); });
    }

    /// This tab is the one on screen. A tab replaced by another is freed a
    /// little later (Aurore's free queue): until then, its timer and its
    /// listeners must not refresh it, or what it sets for the whole screen
    /// (the header's pill) would cover the new tab's.
    bool shown() const;

    /// Entered from the sidebar once already: the focus went to a row.
    bool focused_once() const { return this->focused; }

    /// Calls refresh() every `period_ms` while the app is in the foreground,
    /// this tab is the one shown and its screen is the top one (not under a
    /// dialog or another screen). Tabs are deleted when another one is
    /// opened, which stops it.
    void enable_auto_refresh(int period_ms);

  private:
    aurore::RepeatingTimer auto_timer;
    std::string screen; ///< "tab/overview": how the frame statistics name this tab
    bool focused  = false;
    bool revealed = false; ///< risen into place once (ui/motion)

    void update_ro_note();
};

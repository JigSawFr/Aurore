// ui::hooks — what Cadran asks of the app above it (the main screen, the
// tabs, its identity, saving the preferences) without knowing it: the app
// fills this in once at start-up, before any screen opens. Each hook left
// empty does nothing (or answers nothing).
// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <borealis.hpp>

#include <borealis/cadran/ui/cadran_frame.hpp>

namespace ui::hooks
{

struct Hooks
{
    // The main screen.
    std::function<bool(brls::Activity* activity)> is_main; // `activity` is it
    std::function<void(brls::Activity* main, int position, const std::string& row)> open_tab;
    std::function<void(brls::Activity* main)> update_title;
    // A new main screen, pushed, on tab `position`, the focus on `row`.
    std::function<void(int position, const std::string& row)> push_main;

    // The tabs: re-read the one shown, the name of the screen shown and of
    // the main screen's tab ("tab/about"), and whether a view is a tab.
    std::function<void()> refresh_shown;
    std::function<std::string()> shown_screen;
    std::function<std::string()> main_screen;
    std::function<bool(brls::View* view)> is_tab;

    // The sidebar position of the tab whose screen name is `screen`, -1 when
    // there is none; and the position to fall back on.
    std::function<int(const std::string& screen)> tab_position;
    std::function<int()> settings_tab;
    // The rail's icons (cadran/view/cadran_icon), one per sidebar position,
    // "" for a separator.
    std::function<std::vector<std::string>()> rail_icons;

    // The app: its version and commit (the header's badge), whether it is a
    // release, and whether it writes nothing for now (a read-only mode: the
    // header's pill says so).
    std::function<std::string()> version;
    std::function<std::string()> commit;
    std::function<bool()> is_release;
    std::function<bool()> read_only;
    // The app's logo, drawn in a `w` × `h` box at (0, 0) (CadranIcon "mark":
    // the header's, the about screen's). Unset: Aurore's.
    std::function<void(NVGcontext* vg, float w, float h)> mark;
    // A mode of the app the main screen's pill says over every screen: fills
    // `mode` in and returns true while it lasts (parental controls paused,
    // unlocked…). Unset or false: the screen's own words, or read-only.
    std::function<bool(cadran::PillMode& mode)> pill;
    // The icons of the lines of the app's own screens that Cadran dresses
    // (a DetailCell's id, a CadranIcon's name), besides Aurore's.
    std::function<std::vector<std::pair<const char*, const char*>>()> screen_icons;
    // The title of the main screen's tab `screen` ("tab/about"), for the
    // crumb of a screen opened from it. Unset: none.
    std::function<std::string(const std::string& screen)> tab_title;
    // Today, "2026-10-08", by the app's own clock (the console's user clock).
    // Unset: the system's local date.
    std::function<std::string()> today;

    // The folder of the user's own theme files (*.theme, docs/themes.md),
    // listed by the theme picker after Aurore's presets: the app's, such as
    // "sdmc:/config/<app>/themes". Unset: the presets only.
    std::function<std::string()> themes_dir;

    // Saves ui::prefs() (the language, theme, text size and effects chosen):
    // false when it cannot be saved at all. Unset: the choice lasts until the
    // app quits.
    std::function<bool()> save_prefs;
    // Send a report, offered by a failure's dialog. Unset: not offered.
    std::function<void()> send_report;

    // A flag of the app's environment, for its tests (`name`: "STILL_FOCUS",
    // "CADRAN_NOGLOW", "FOCUS_LOG", "CUT_TEXT", "DOCKED", "BATTERY"). Unset:
    // the process environment's BRLS_<name> ("1", "true" or "yes"), in a
    // build with BRLS_DEV_TOOLS (not a release one, by default), never on
    // the console.
    std::function<bool(const char* name)> env_flag;
};

// The one set, filled in at start-up.
Hooks& get();

} // namespace ui::hooks

// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-FileCopyrightText: 2026 Taylor
// SPDX-License-Identifier: GPL-3.0-or-later

// ui — Cadran's helpers for dialogs, toasts, pickers, sheets, keyboards,
// visibility and focus. The app adds its own (background tasks, its
// settings file, its error codes) in the same namespace.
#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>
#include <aurore.hpp>

#include <aurore/cadran/ui/cadran_text.hpp>
#include <aurore/cadran/ui/format_text.hpp>
#include <aurore/cadran/ui/guarded.hpp>
#include <aurore/cadran/ui/prefs.hpp>
#include <aurore/cadran/view/cadran_icon.hpp>

namespace ui
{

// The status colours below are the boards' inks (cadran::install_theme(),
// which must run before the first lookup: Aurore aborts on an unknown
// theme key), one shade per theme (>= 4.5:1).

/// On the console: the default font of the labels made from now on, for the
/// strings' language `locale`: the system's Latin / Japanese font, the Chinese
/// one only as a fallback; for Chinese, the Chinese one. Right after
/// createWindow(), before the first label exists, then again when the language
/// changes while the app runs (live_prefs, which builds the screens again).
void use_font_for(const std::string& locale);

/// What a toast says: news (plain), something done (a check), or something
/// not done because of how things are (a warning); the icon in the Cadran look.
enum class Toast
{
    Info,
    Done,
    Refused,
};
/// A toast of `kind`. Deferred to the next frame (toasts fired right after a
/// system applet returns were dropped on fw 22.1.0).
void notify(const std::string& text, Toast kind = Toast::Info);
/// Something asked that was not done, and why, as a Refused toast (its
/// warning icon): a refusal is said and gone, a failure is a dialog (error).
void refuse(const std::string& text);
/// A failure the user asked for something and did not get: a dialog, not a
/// toast (a toast is gone before "error 0x…" is read). Next frame, so it can
/// be called from a dialog button. Logged as "error: " for the smoke test.
void error(const std::string& text);
/// `what` failed because of `why` (a path, a message from the network…).
void error(const std::string& what, const std::string& why);
/// The dialog the three open, now: the title, the reason, the code small, and
/// Send a report (OK focused) when `f.report`.
void show_failure(const cadran_text::Failure& f);
/// `next` in place of the current screen. Aurore never pops the first
/// activity (the loading screen, under every start screen): a screen that
/// cannot be popped gets `next` pushed over it instead.
void replace_screen(aurore::Activity* next);
/// Two-button dialog. Cancel (left) has the focus when it opens;
/// `confirm_label` (right) runs `on_yes`, Cancel or B runs `on_no`.
/// `danger`: the confirm button is drawn in the "bad" colour (see below).
void confirm(const std::string& body, const std::string& confirm_label,
    std::function<void()> on_yes, std::function<void()> on_no = nullptr, bool danger = false);
/// Same, for actions that delete something: the confirm button is drawn in the
/// "bad" colour so it never looks like an ordinary OK.
void confirm_danger(const std::string& body, const std::string& confirm_label, std::function<void()> on_yes);
void info(const std::string& body);
/// A web page to open on a phone: `body`, the QR code of `url`, then `url`
/// itself, shortened (text::short_url), and OK.
void qr_dialog(const std::string& body, const std::string& url);

/// A list of checks in a dialog: each line with its mark (a teal check for
/// what is ready, an amber or coral triangle for what is not, a grey dot for
/// what is only said), under `title`, `note` small at the end (may be empty).
enum class Check
{
    Ok,
    Warn,
    Bad,
    Info,
    Header, ///< not a check: the name of the panel of lines after it
};
/// A line of checklist_body().
struct CheckLine
{
    Check state = Check::Info;
    std::string text;
};
/// One of them: the mark, then the text (`size` px).
aurore::Box* check_row(const CheckLine& line, float size = 17);
/// Under `title` (and `subtitle`), the lines in rounded panels, one per
/// Header line (its name small above it); `note` with an info icon last.
aurore::Box* checklist_body(const std::string& title, const std::vector<CheckLine>& lines, const std::string& note = "",
    const std::string& subtitle = "");
/// A dialog that only says something: a badge (`icon` in `tone`'s colour),
/// `title`, `text`, OK.
void notice(const char* icon, Check tone, const std::string& title, const std::string& text);
/// `body` with Cancel and `hold_label`: A (or a finger) held on it for 2 s,
/// the button filling meanwhile, runs `on_yes`; let go sooner, it empties
/// and nothing happens. For what is hard to undo from the console. The dialog
/// says how itself (hold_hint, the A key for a controller): `body` need not.
void hold_confirm(aurore::Box* body, const std::string& hold_label, std::function<void()> on_yes,
    std::function<void()> on_no = nullptr);

/// A dialog showing `text`, laid out by cadran::dialog_body (its question as
/// the title, its tables, its notes), the caller adding its buttons; a text
/// taller than the dialog's room scrolls.
aurore::Dialog* dialog(const std::string& text);
/// The same with `below` under the text, the card's width unless it aligns
/// itself: a QR code, a code to read out, the cards to support the app.
aurore::Dialog* dialog(const std::string& text, aurore::View* below);
/// The same for a body already laid out (a checklist, a sheet): as it is, or
/// in a scrolling frame when taller than the dialog's room.
aurore::Dialog* box_dialog(aurore::Box* body);

/// B closes `dialog` and runs `on_cancel` (Aurore's own B only closes it).
void on_cancel(aurore::Dialog* dialog, std::function<void()> on_cancel);
/// A Cancel button and B, both closing `dialog` and running `cancel`: a
/// dialog that waits for something (a check, an answer from the network).
void add_cancel(aurore::Dialog* dialog, std::function<void()> cancel);

/// A row's colours in pick()'s `swatches` (CadranSwatch).
using Swatch = CadranSwatch::Colors;

/// A list of values, as a sheet (ui/action_sheet.cpp); `on_pick(index)` runs
/// after it has closed. The row
/// `selected` has the focus and is checked as the current value, unless
/// `checked` is false (a list of things to do, not of values). `details`, when
/// given, says more about each row, after its label and quieter; `icons`, what
/// goes before each row's label: a Cadran icon's name ("moon"), or a picture
/// (a resource, "img/…png", drawn as it is); empty: none. `swatches`, when
/// given, a sample of colours in the icon's place instead, for the rows it
/// has (a palette's, as pick_palette() shows them): its background in a
/// rounded square, its ring inside, its motif (a Cadran icon's name) in its
/// ink, a hairline of the theme's cadran/line around it (CadranSwatch).
void pick(const std::string& title, const std::vector<std::string>& values, int selected,
    std::function<void(int)> on_pick, bool checked = true, const std::vector<std::string>& details = {},
    const std::vector<std::string>& icons = {}, const std::vector<Swatch>& swatches = {});
/// The same as tiles, `columns` to a row, each its Cadran icon (`icons`, a
/// name) large above its label: a few values that read better side by side
/// (the tabs of Start on).
void pick_grid(const std::string& title, const std::vector<std::string>& values, const std::vector<std::string>& icons,
    int selected, std::function<void(int)> on_pick, int columns = 4);
/// The same with several rows checked: those `marks` sets (true). Choosing a
/// row closes the list as pick() does: to toggle one, the caller opens it
/// again with the row's mark changed and the focus on it (`focus`).
void pick_marked(const std::string& title, const std::vector<std::string>& values, const std::vector<bool>& marks,
    int focus, std::function<void(int)> on_pick, const std::vector<std::string>& details = {});
/// A list of things to do, as a sheet (ui/action_sheet.cpp): each line's
/// label, and what it runs once the sheet has closed; its icon (a Cadran
/// icon, nullptr: none), what is set now at its end (`detail`), its group
/// (a section starts where it changes, its name the kicker above it) and its
/// tone (ON: the detail in teal, WARN: amber, DANGER: a removal, in coral);
/// switched(): a switch at its end, on or off, instead of the detail. A line
/// without `run` only closes the sheet. `subtitle`, under the title: what it
/// is about now; `image`, an NVG image before the title (someone else's,
/// kept). The first line is selected; none, no sheet.
struct Choice
{
    enum class Tone
    {
        PLAIN,
        ON,
        WARN,
        DANGER
    };
    Choice(std::string text, std::function<void()> action, const char* icon_name = nullptr, std::string detail_text = "",
        std::string group_name = "", Tone line_tone = Tone::PLAIN)
        : label(std::move(text))
        , run(std::move(action))
        , icon(icon_name)
        , detail(std::move(detail_text))
        , group(std::move(group_name))
        , tone(line_tone)
    {
    }
    Choice& switched(bool on)
    {
        this->toggle = on ? 1 : 0;
        return *this;
    }
    std::string label;
    std::function<void()> run;
    const char* icon;
    std::string detail;
    std::string group;
    Tone tone;
    int toggle = -1; ///< switched(): 1 on, 0 off
};
/// The sheet of `choices` (see Choice) under `title`.
void pick_actions(const std::string& title, const std::vector<Choice>& choices, const std::string& subtitle = "",
    int image = 0);
/// The sheet those open: its view, on top of the activities' stack.
bool is_sheet(aurore::View* view);

/// The system keyboard for a text. The callback only on confirm.
void prompt_text(const std::string& header, const std::string& initial, int max_len,
    std::function<void(std::string)> on_value);

/// An action that writes stays visible when the app cannot write (a
/// read-only mode), greyed (title and value), so the user sees what exists.
/// `title_key` / `detail_key`: the theme colours when writable. A
/// BooleanCell gets its own colours back from its state.
void show_writable(aurore::DetailCell* cell, bool writable, const char* title_key = "cadran/text",
    const char* detail_key = "cadran/text2");

/// Hides a view when `visible` is false (Visibility::GONE frees its space).
/// When the hidden view had the focus, the focus moves to the nearest visible
/// focusable sibling (the next one on a tie), so it never lands on the list
/// container or jumps far away.
void set_visible(aurore::View* view, bool visible);

/// Applies several visibility changes, showing views before hiding others, so a
/// cell that replaces the focused one (Unlock -> Lock now) is there to take
/// the focus.
void set_visible_all(std::initializer_list<std::pair<aurore::View*, bool>> changes);

/// A label's text, or a cell's detail, set only when it changes: Aurore lays
/// the whole screen out again on every setText, and the tabs that refresh
/// themselves every few seconds mostly write the same values back.
void set_text(aurore::Label* label, const std::string& text);
void set_detail(aurore::DetailCell* cell, const std::string& text);

/// Runs `fill`, which adds many views to `box` (or changes them), with the box
/// out of the screen's layout, then lays the screen out once. Aurore lays
/// the whole screen out again on each addView and setText: a list of n rows
/// built in place cost n layouts of the whole screen, n of them on screen.
void batch_layout(aurore::Box* box, const std::function<void()>& fill);

/// For an activity's onResume(): Aurore then gives the focus back to the
/// view that opened the dialog (or screen) just closed, and gives none when
/// that view was hidden meanwhile; the focus would stay on the closed
/// dialog's button, deleted after its animation, and no button answer any
/// more. In that case only, `fallback` (else the activity's first focusable
/// view) gets it first.
void keep_focus(aurore::Activity* activity, aurore::View* fallback = nullptr);

/// Whether `activity` has the focus: nothing (a dialog, another screen) is
/// open over it. Unlike the top of Aurore's stack, true again as soon as a
/// dialog over it starts closing.
bool has_focus(aurore::Activity* activity);
/// Whether `activity` is the top of Aurore's stack: no screen or dialog
/// pushed over it, one closing included.
bool on_top(aurore::Activity* activity);
/// The main screen, wherever it is in the stack; nullptr before it opens.
aurore::Activity* main_screen();
/// Shows its tab `position` (ui::tab) and focuses its content: the view with
/// id `row` when given and shown, else its first one. Nothing before it opens.
void open_tab(int position, const std::string& row = "");
/// Re-reads the tab on screen, if any (after the read-only or developer mode
/// changed, the language or the text size).
void refresh_shown_tab();

/// Opens another tab of the main screen and focuses its first item. `from` is
/// any view inside the current tab; the switch happens on the next frame
/// because the current tab is deleted by it.
void go_to_tab(aurore::View* from, int position);
/// `content` (a tab's view, such as the network clock) on a screen of its own,
/// over the current one, titled `title`; B closes it.
void open_screen(aurore::View* content, const std::string& title);

/// The language and theme pickers, shared by Preferences and the main
/// screen's header: save the choice (ui::prefs, through
/// hooks::get().save_prefs) and apply it at once (a language makes every
/// screen again, live_prefs; a theme or the text size runs `after`, a
/// refresh). language_name / theme_name: the choice saved, as text;
/// text_size_name, language_name and theme_name their shorter words on a tile
/// (Preferences). The theme picker ends with the palette's row, which opens
/// pick_palette: the boards' colours, Aurore's presets (aurore::Theme::
/// builtinThemes) or the user's theme files (ui::hooks' themes_dir);
/// palette_name, the one saved (ui::Prefs::theme_file), by its name.
/// @{
void pick_language(std::function<void()> after = nullptr);
void pick_theme(std::function<void()> after = nullptr);
void pick_palette(std::function<void()> after = nullptr);
std::string palette_name();
void pick_text_size(std::function<void()> after = nullptr);
std::string text_size_name(bool tile = false);
/// @}
/// The main screen's sidebar (ui::Prefs::rail): open, on its icons, or
/// folding by itself; applied at once (cadran::apply_rail). rail_name: the
/// choice saved, as text.
/// @{
void pick_rail(std::function<void()> after = nullptr);
std::string rail_name();
/// @}
/// How close the screens' pieces stand (ui::Prefs::density): automatic,
/// comfortable, compact or dense; the screens built again at once
/// (live_prefs::density_changed). density_name: the choice saved, as text.
/// @{
void pick_density(std::function<void()> after = nullptr);
std::string density_name();
/// @}
/// The language and the theme saved, as text (shorter on a tile).
/// @{
std::string language_name(bool tile = false);
std::string theme_name(bool tile = false);
/// @}
/// − on the main screen: a list of the two, each opening its picker.
void quick_settings(std::function<void()> after = nullptr);

/// Saves ui::prefs() through hooks::get().save_prefs: false when it cannot be
/// saved at all.
bool save_prefs();

} // namespace ui

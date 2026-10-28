// Cadran — Aurore's look, after PlayGuard's and PlayShelf's 2026 design
// boards: the navy and cool-white palettes, an icon rail for a sidebar, the
// app's mark and a status pill in the header, cards in place of lists, the
// figures in Sora. These set up the frame every screen shares; an app's tabs
// draw their own boards, cadran::List dresses the lists, cadran/ui/cadran_kit
// and cadran/ui/cadran_modal hold the pieces.
// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#pragma once

#include <functional>
#include <string>
#include <aurore.hpp>

namespace cadran
{
/// The boards' two palettes on Aurore's two variants (dark: navy; light:
/// cool white), the rail's metrics and the display font: before any view.
void install_theme();
/// A frame's header after the boards: the mark for the app icon, the title
/// at 24, a pill at the right (`icon` and `text`; `filled` teal, else a
/// bordered one) and, when asked, the date.
void style_header(aurore::AppletFrame* frame, const char* icon, const std::string& text, bool filled, bool with_date);
/// From then on, every screen but the main one gets the header after the
/// boards (the mark, the title) and its list in cards (cadran::List), the
/// first time it takes the focus.
void install_screens();
/// A TabFrame's sidebar as the boards' rail: an icon before each label, the
/// active tab on a teal pill; open, on its icons, or folding by itself as
/// ui::prefs() says (ui::RAILS).
void style_rail(aurore::View* tab_frame);
/// ui::prefs().rail changed (an app's Preferences › Sidebar): the main
/// screen's rail after it, at once.
void apply_rail();
/// Both, for the main screen.
void style_frame(aurore::AppletFrame* frame, aurore::View* tab_frame);
/// Whether `frame` is a main screen: it holds a TabFrame (or a view with the
/// id "main_tabs"). install_screens() styles it with style_frame() the first
/// time it takes the focus, unless the app did already.
bool is_main_frame(aurore::View* frame);
/// Whether `frame` is a screen of its own, not a step from the one under it
/// (a child's screen): no "Console ›" crumb before its title, no version
/// badge after it. The app says so with ui::hooks' standalone_screen, or by a
/// view with the id "cadran/standalone" in the screen.
bool is_standalone_screen(aurore::View* frame);
/// The main screen's header pill: its text, or hidden when empty (each
/// screen says what it has to: "Library up to date", "Play timer on"…), its
/// icon, and its tone: amber for a state that needs the user, coral for
/// something off that should be on. While the app writes nothing (ui::hooks'
/// read_only), the pill says so instead, whatever the screen.
enum class Tone
{
    OK,
    WARN,
    DANGER
};
/// A mode of the app the main screen's pill says over what every screen
/// says, as long as it lasts (ui::hooks' pill fills it in; read-only is
/// Aurore's own): its text, icon and tone, and what a tap on the pill opens
/// (nullptr: nothing). PlayGuard's: parental controls paused or unlocked.
struct PillMode
{
    std::string text;
    std::string icon = "check";
    Tone tone        = Tone::WARN;
    std::function<void()> on_tap;
};
void set_pill(const std::string& text, const char* icon = "check", Tone tone = Tone::OK);
/// The pill again after a mode changed (ui::hooks' pill, read-only).
void refresh_pill();
/// The soft glow of the boards, an absolute full-screen view to add first in
/// a frame: a radial gradient of `color_key` centred on (cx, cy), `radius`
/// wide. Nothing with the lighter effects (cadran/ui/effects), without the
/// glow (ui::prefs) or with AURORE_CADRAN_NOGLOW=1 (ui::env_flag).
aurore::View* make_glow(float cx, float cy, float radius, const char* color_key);
}

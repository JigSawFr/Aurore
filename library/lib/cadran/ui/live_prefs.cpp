// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

#include <aurore/cadran/ui/live_prefs.hpp>

#include <functional>
#include <aurore.hpp>

#include <aurore/cadran/ui/hooks.hpp>
#include <aurore/cadran/ui/prefs.hpp>
#include <aurore/cadran/ui/ui.hpp>

namespace live_prefs
{
namespace
{
    aurore::ThemeVariant s_system_theme = aurore::ThemeVariant::DARK;
    std::string s_system_locale; // the console's language
    std::string s_locale; // the strings' language now
    float s_app_density = aurore::DENSITY_COMFORTABLE; // the app's (or AURORE_DENSITY's)

    // The tab shown, as a sidebar position (ui::hooks::tab_position), the
    // settings' tab when none is known.
    int shown_tab()
    {
        const auto& h            = ui::hooks::get();
        const std::string screen = h.shown_screen ? h.shown_screen() : "";
        const int position       = h.tab_position ? h.tab_position(screen) : -1;
        if (position >= 0)
            return position;
        return h.settings_tab ? h.settings_tab() : 0;
    }

    // Every screen above the loading one closed, one at a time; then `next`.
    void close_all(std::function<void()> next)
    {
        if (!aurore::Application::popActivity(aurore::TransitionAnimation::NONE, [next]()
                { close_all(next); }))
            next();
    }

    // The focused line of the shown tab: its id (or its nearest parent's that
    // has one), "" when the focus is elsewhere (the rail, the header).
    std::string focused_row()
    {
        const auto& is_tab = ui::hooks::get().is_tab;
        if (!is_tab)
            return "";
        for (aurore::View* v = aurore::Application::getCurrentFocus(); v; v = v->getParent())
        {
            if (is_tab(v))
                return "";
            if (!v->getId().empty())
            {
                for (aurore::View* up = v->getParent(); up; up = up->getParent())
                    if (is_tab(up))
                        return v->getId();
                return "";
            }
        }
        return "";
    }

    // Every screen made again, the main one on the tab that was shown, the focus
    // back on its line: the screens above it (a sub-screen the header's setting
    // was changed from) go. An app that cannot build its main screen again
    // (ui::hooks' push_main unset) keeps the screens open: the screens opened
    // from then on take the change, and the one shown is read again.
    void rebuild()
    {
        if (!ui::hooks::get().push_main)
        {
            aurore::sync([]()
                {
            if (ui::hooks::get().refresh_shown) ui::hooks::get().refresh_shown(); });
            return;
        }
        aurore::sync([]()
            {
        const int tab = shown_tab();
        const std::string row = focused_row();
        close_all([tab, row]() {
            if (ui::hooks::get().push_main) ui::hooks::get().push_main(tab, row);
        }); });
    }

} // namespace

void start()
{
    // Again (Application::init, then an app of its own): the console's
    // language is the one read the first time.
    static bool done = false;
    if (done)
        return;
    done = true;
    // The platform read the console's language (the app leaves Aurore's
    // locale on auto), and the strings are in it: the preferences' instead.
    s_system_theme            = aurore::Application::getPlatform()->getThemeVariant();
    s_app_density             = aurore::getDensity();
    s_system_locale           = aurore::Application::getLocale();
    const std::string& chosen = ui::prefs().language;
    s_locale                  = chosen == "system" ? s_system_locale : chosen;
    if (s_locale != s_system_locale)
        aurore::loadTranslations(s_locale);
}

void apply_theme()
{
    ui::Prefs& p                 = ui::prefs();
    aurore::ThemeVariant variant = p.theme == "dark" ? aurore::ThemeVariant::DARK
        : p.theme == "light"                         ? aurore::ThemeVariant::LIGHT
                                                     : s_system_theme;
    if (p.theme_file.empty() || !aurore::Theme::loadFile(p.theme_file))
        aurore::Theme::reset();
    // A file for one variant only: that one, which the header's button and
    // the pickers then show too (the pickers keep the two in step; older
    // preferences may not).
    if (!aurore::Theme::styles(variant))
    {
        variant = variant == aurore::ThemeVariant::DARK ? aurore::ThemeVariant::LIGHT : aurore::ThemeVariant::DARK;
        if (p.theme != "system")
            p.theme = variant == aurore::ThemeVariant::DARK ? "dark" : "light";
    }
    aurore::Application::getPlatform()->setThemeVariant(variant);
}

std::string locale()
{
    return s_locale;
}

void language_changed()
{
    const std::string& chosen = ui::prefs().language;
    s_locale                  = chosen == "system" ? s_system_locale : chosen;
    aurore::loadTranslations(s_locale);
    ui::use_font_for(s_locale); // Chinese has its own (made again with the screens)
    rebuild();
}

void apply_density()
{
    const std::string& chosen = ui::prefs().density;
    float density             = s_app_density;
    if (chosen == "comfortable")
        density = aurore::DENSITY_COMFORTABLE;
    else if (chosen == "compact")
        density = aurore::DENSITY_COMPACT;
    else if (chosen == "dense")
        density = aurore::DENSITY_DENSE;
    else if (s_app_density == aurore::DENSITY_COMFORTABLE
        && aurore::Application::getWidthClass() == aurore::WidthClass::COMPACT)
        density = aurore::DENSITY_DENSE; // "auto" on a small screen
    aurore::setDensity(density);
}

void density_changed()
{
    apply_density();
    rebuild();
}
} // namespace live_prefs

// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-FileCopyrightText: 2026 Taylor
// SPDX-License-Identifier: GPL-3.0-or-later

#include <aurore/cadran/view/cadran_tab.hpp>

#include <aurore/cadran/ui/hooks.hpp>
#include <aurore/cadran/ui/motion.hpp>
#include <aurore/cadran/ui/ui.hpp>

using namespace aurore::literals;

namespace
{
CadranTab* s_shown = nullptr;
std::string s_main_screen; // main_screen()
} // namespace

CadranTab::CadranTab(const std::string& xml_res, bool refreshes)
{
    this->inflateFromXMLRes(xml_res);
    // "xml/tab/overview.xml" -> "tab/overview"
    this->screen = xml_res.substr(xml_res.rfind("xml/") == 0 ? 4 : 0);
    if (this->screen.size() > 4 && this->screen.compare(this->screen.size() - 4, 4, ".xml") == 0)
        this->screen.resize(this->screen.size() - 4);
    if (refreshes)
        this->registerAction("hints/refresh"_i18n, aurore::BUTTON_X, [this](aurore::View*)
            {
            this->refresh();
            return true; });
    if (auto* note = dynamic_cast<aurore::Label*>(this->getView("tab_ro_note")))
        note->setSingleLine(false);
}

CadranTab::~CadranTab()
{
    if (s_shown == this)
        s_shown = nullptr;
}

void CadranTab::onChildFocusGained(aurore::View* directChild, aurore::View* focusedView)
{
    aurore::Box::onChildFocusGained(directChild, focusedView);
    this->focused = true;
}

void CadranTab::update_ro_note()
{
    const auto& hooks = ui::hooks::get();
    if (auto* note = dynamic_cast<aurore::Label*>(this->getView("tab_ro_note")))
        ui::set_visible(note, hooks.read_only && hooks.read_only());
}

void CadranTab::willAppear(bool resetState)
{
    aurore::Box::willAppear(resetState);
    s_shown = this;
    for (aurore::View* v = this; v; v = v->hasParent() ? v->getParent() : nullptr)
        if (v->getId() == "main_tabs")
            s_main_screen = this->screen;
    if (ui::hooks::get().tab_shown)
        ui::hooks::get().tab_shown(this->screen);
    this->update_ro_note();
    this->before_refresh();
    this->refresh();
    // Opened (not back from a dialog or a screen over it): the tab rises into
    // place, its lines in cascade after it (cadran::List::apply).
    if (!this->revealed)
    {
        this->revealed = true;
        motion::reveal(this, 6.0f, 160);
    }
    this->appeared();
}

bool CadranTab::shown() const
{
    return s_shown == this;
}

std::string CadranTab::shown_screen()
{
    return s_shown ? s_shown->screen : "";
}

std::string CadranTab::main_screen()
{
    return s_main_screen;
}

void CadranTab::refresh_shown()
{
    if (!s_shown)
        return;
    s_shown->update_ro_note();
    s_shown->refresh();
}

void CadranTab::enable_auto_refresh(int period_ms)
{
    this->auto_timer.setPeriod(period_ms);
    this->auto_timer.setCallback([this]()
        {
        const auto& hooks = ui::hooks::get();
        if (hooks.in_focus && !hooks.in_focus()) return;
        if (!this->shown()) return; // replaced, waiting to be freed
        if (!ui::on_top(this->getParentActivity())) return;
        this->refresh(); });
    this->auto_timer.start();
}

// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

// ToolChip — one of a tab's controls as a small pill in a row (a server, a
// search, an order, a filter, a section): its icon, what it sets in dim
// words, what it is set to. A on it runs its action. In teal when it narrows
// what the tab shows (a search, a filter). The tab's list keeps the room
// below it.
#pragma once

#include <string>
#include <aurore.hpp>

class CadranIcon;

class ToolChip : public aurore::Box
{
  public:
    ToolChip(const char* icon_name, const std::string& what);

    /// What it is set to; set only when it changes (a label set lays the
    /// screen out again).
    void set_value(const std::string& text);
    /// In teal: it narrows what the tab shows.
    void set_active(bool on);

  private:
    CadranIcon* icon     = nullptr;
    aurore::Label* what  = nullptr;
    aurore::Label* value = nullptr;
    bool active          = false;
};

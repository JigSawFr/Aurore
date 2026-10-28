// CadranTab — the demo's first tab: Cadran's pieces (cards, pills, chips,
// digits, a bar, icons), its dialogs, sheets and toasts, and its Markdown,
// each built in code.
// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#pragma once

#include <aurore.hpp>
#include <aurore/cadran/view/scroll_view.hpp>

class CadranTab : public ScrollView
{
  public:
    CadranTab();

    static brls::View* create();
};

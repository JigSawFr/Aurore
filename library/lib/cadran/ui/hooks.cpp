// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

#include <aurore/cadran/ui/hooks.hpp>

namespace ui::hooks
{

Hooks& get()
{
    static Hooks hooks;
    return hooks;
}

} // namespace ui::hooks

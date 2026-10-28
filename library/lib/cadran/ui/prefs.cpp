// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#include <aurore/cadran/ui/hooks.hpp>
#include <aurore/cadran/ui/prefs.hpp>

#include <cctype>
#include <cstdlib>
#include <string>

namespace ui
{

namespace
{
    Prefs s_defaults;
    Prefs* s_prefs = &s_defaults;
} // namespace

Prefs& prefs()
{
    return *s_prefs;
}

void use_prefs(Prefs& p)
{
    s_prefs = &p;
}

bool env_flag(const char* name)
{
    if (hooks::get().env_flag)
        return hooks::get().env_flag(name);
#if defined(__SWITCH__) || !defined(AURORE_DEV_TOOLS)
    (void)name;
    return false;
#else
    const char* v = std::getenv((std::string("AURORE_") + name).c_str());
    if (!v)
        return false;
    std::string value(v);
    for (char& c : value)
        c = (char)std::tolower((unsigned char)c);
    return value == "1" || value == "true" || value == "yes";
#endif
}

} // namespace ui

// tr — aurore::getStr and "…"_i18n for the text helpers that only translate
// (format_text.cpp): borealis' own. Built with AURORE_CADRAN_HOST_I18N
// (a host suite, without borealis' views), the same two over a
// aurore::internal::getRawStr the suite defines, from a catalog of its own.
// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#pragma once

#ifdef AURORE_CADRAN_HOST_I18N
#include <fmt/format.h>
#include <cstddef>
#include <exception>
#include <string>

namespace aurore
{
namespace internal
{
    std::string getRawStr(std::string stringName);
} // namespace internal

/// As borealis' (core/i18n.hpp): a string that does not take the arguments
/// reads as its key.
template <typename... Args>
std::string getStr(std::string stringName, Args&&... args)
{
    const std::string raw = internal::getRawStr(stringName);
    try
    {
        return fmt::format(fmt::runtime(raw), args...);
    }
    catch (const std::exception&)
    {
        return stringName;
    }
}

inline namespace literals
{
    std::string operator""_i18n(const char* str, size_t len);
} // namespace literals
} // namespace aurore
#else
#include <aurore.hpp>
#endif

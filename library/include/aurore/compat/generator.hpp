// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

/* aurore::generator: std::generator where the standard library has it, else
 * Lewis Baker's and Corentin Jabot's reference implementation (aurore/extern,
 * in aurore::compat). libc++ has none yet (web, Android, Apple, PS4), nor
 * libstdc++ before GCC 14. aurore::elements_of(r) yields all of r, for
 * either. AURORE_COMPAT_FALLBACK takes the bundled one everywhere. */
#pragma once

#include <utility>
#include <version>

#if defined(__cpp_lib_generator) && !defined(AURORE_COMPAT_FALLBACK)

#include <generator>

namespace aurore
{
using std::generator;

template <class Range>
constexpr auto elements_of(Range&& range)
{
    return std::ranges::elements_of<Range&&> { std::forward<Range>(range) };
}
} // namespace aurore

#else

#include <aurore/extern/generator/generator.hpp>

namespace aurore
{
using compat::generator;

template <class Range>
constexpr auto elements_of(Range&& range)
{
    return compat::ranges::elements_of<Range>(std::forward<Range>(range));
}
} // namespace aurore

#endif

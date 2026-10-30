// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

/* aurore::move_only_function: std::move_only_function where the standard
 * library has it, else nontype_functional's (aurore/extern), the reference
 * implementation of P0288. libc++ has none yet (web, Android, Apple, PS4).
 * AURORE_COMPAT_FALLBACK takes the bundled one everywhere (its tests). */
#pragma once

#include <functional>
#include <version>

#if defined(__cpp_lib_move_only_function) && !defined(AURORE_COMPAT_FALLBACK)

namespace aurore
{
using std::move_only_function;
} // namespace aurore

#else

#include <aurore/extern/nontype_functional/std23/move_only_function.h>

namespace aurore
{
using std23::move_only_function;
} // namespace aurore

#endif

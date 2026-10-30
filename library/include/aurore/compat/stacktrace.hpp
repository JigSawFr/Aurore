// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

/* aurore::stacktrace: std::stacktrace where the standard library has it, and
 * AURORE_HAS_STACKTRACE says whether it does (1) or not (0). libc++ has none
 * (web, Android, Apple, PS4); no stand-in is bundled, since the libraries
 * that read a stack (cpptrace, libbacktrace) need files and debug data the
 * consoles do not have. With GCC 13 and 14, the program links -lstdc++exp. */
#pragma once

#include <version>

#if defined(__cpp_lib_stacktrace)

#include <stacktrace>

#define AURORE_HAS_STACKTRACE 1

namespace aurore
{
using std::stacktrace;
} // namespace aurore

#else

#define AURORE_HAS_STACKTRACE 0

#endif

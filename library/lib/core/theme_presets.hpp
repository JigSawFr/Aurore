// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <ctime>
#include <string>
#include <vector>

namespace aurore
{

// A theme file built into the library: "builtin:<id>"
struct BuiltinTheme
{
    const char* id;
    const char* text; // nullptr: made when read (seasonalThemeText)
};

const std::vector<BuiltinTheme>& builtinThemeTexts();

// The preset's text: its own, or the one made for now
std::string builtinThemeText(const BuiltinTheme& theme, std::time_t now);

// The id of the preset builtin:seasonal uses at `now` (in local time)
const char* seasonalThemeId(std::time_t now);

} // namespace aurore

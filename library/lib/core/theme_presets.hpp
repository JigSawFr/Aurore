/*
    Copyright 2026 JigSawFr

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

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

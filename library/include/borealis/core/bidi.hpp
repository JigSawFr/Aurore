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

#include <string>

namespace brls
{

// `text` in the order it is drawn: the right-to-left runs (Hebrew, Arabic…)
// reversed, mirrored brackets swapped and Arabic letters joined through their
// presentation forms, line by line. Needs USE_FRIBIDI; without it, or for a
// text with no right-to-left letter, `text` as it is.
// Each line is reordered as a whole: a right-to-left paragraph that wraps
// keeps its lines in reading order but not the words across them.
std::string bidiVisual(const std::string& text);

// Built with USE_FRIBIDI.
bool bidiAvailable();

} // namespace brls

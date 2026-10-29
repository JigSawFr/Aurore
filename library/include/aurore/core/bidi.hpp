// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>

namespace aurore
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

} // namespace aurore

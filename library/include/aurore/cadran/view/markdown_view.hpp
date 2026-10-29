// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

// markdown_view — util/markdown.hpp's blocks as views: headings in the
// accent colour (the first two levels over a rule, with their emoji when
// Twemoji has it), paragraphs, bullet, numbered and task lists (a drawn box,
// ticked or not), code on a tinted card, quotes and GitHub's alerts with a
// coloured bar and title, rules, and tables as a grid. Labels only: nothing
// is opened or run.
#pragma once

#include <vector>
#include <aurore.hpp>

#include <aurore/cadran/util/markdown.hpp>

namespace markdown_view
{

/// Adds `blocks` (markdown::parse) to `box` as labels: headings, lists, code,
/// quotes and alerts, rules, tables. Nothing is opened or run.
void fill(aurore::Box* box, const std::vector<markdown::Block>& blocks);

} // namespace markdown_view

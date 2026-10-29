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

#include <functional>
#include <string>
#include <aurore.hpp>
#include <aurore/cadran/ui/cadran_kit.hpp>
#include <aurore/cadran/view/scroll_view.hpp>

// The demo's pages share one grid: these lengths, Cadran's card radii
// (cadran::kit::RADIUS_*) and type scale (cadran::kit::TEXT_*).
namespace show
{
/// A compact screen (under 720 units wide, aurore::WidthClass::COMPACT):
/// the pages' rows stack their cards, their margins are the tabs' own.
bool compact();

/// A page's margins: Cadran's (40 at the sides, 28 at the top), more at
/// the bottom so that the last card clears the footer; on a compact
/// screen, the tab frame's (@style/aurore/tab_frame/content_padding_*).
float pageTop();
float pageSides();
float pageBottom();
/// A screen of its own (no rail) whose content is not a ScrollView: its
/// sides under the header's mark, where a ScrollView's page puts them.
float screenSides();
/// Between two sections, from a section's kicker to its content, between
/// two cards, inside a card. All of them follow the density (aurore::dp).
float sectionGap();
float headGap();
float gap();
float padV();
float padH();

/// A tab's page: a Cadran ScrollView holding a column with the page's
/// margins, which it returns.
aurore::Box* page(ScrollView* scroll);

/// A section of `page`: its kicker (an icon and its title in capitals), a
/// line saying what it shows (may be empty), then the box returned, a
/// column, for its content.
aurore::Box* section(aurore::Box* page, const char* icon, const std::string& title, const std::string& lead = "");

/// A row (or a column) whose children are gap() apart (`gap` < 0) or
/// `gap` dp; `wrap`: on to the next line when they do not fit. On a compact
/// screen every row wraps, and a card that grows takes a line of its own.
aurore::Box* row(bool wrap = false, float gap = -1);
aurore::Box* column(float gap = -1);

/// A card of the page: Cadran's, padded, a column; `focusable` takes the
/// focus ring, `grow` shares a row with its neighbours.
aurore::Box* card(bool focusable = false, bool grow = true);

/// On a compact screen, `view` shares its row's line with `perLine` - 1
/// others (the row wraps); elsewhere, nothing changes. Returns `view`.
aurore::View* share(aurore::View* view, int perLine);

/// A card's title (TEXT_LEAD) and its note under it (TEXT_NOTE, the second ink).
aurore::Label* title(const std::string& text);
aurore::Label* note(const std::string& text);

/// A button sized by its text (cadran::kit::fit_button), in Cadran's look.
aurore::Button* button(const std::string& text, std::function<void()> run, bool primary = false);

/// The same with a Cadran icon before its text.
aurore::IconButton* iconButton(const char* icon, const std::string& text, std::function<void()> run);

/// A choice among a few, as a tile: its icon (none: nullptr) and its name;
/// `run` when chosen. markChoice() outlines the one in use.
aurore::Box* choice(const char* icon, const std::string& name, std::function<void()> run);
void markChoice(aurore::Box* tile, bool chosen);

/// A few lines of code on the second surface, in the quieter ink.
aurore::Box* code(const std::string& text);

/// A label filling its line and centred in it.
aurore::Label* centred(aurore::Label* label);
} // namespace show

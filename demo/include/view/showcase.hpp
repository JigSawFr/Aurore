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
/// A page's margins: Cadran's (40 at the sides, 28 at the top), more at
/// the bottom so that the last card clears the footer.
constexpr float PAGE_TOP    = 28;
constexpr float PAGE_SIDES  = 40;
constexpr float PAGE_BOTTOM = 48;
/// A screen of its own (no rail) whose content is not a ScrollView: its
/// sides under the header's mark, where a ScrollView's page puts them.
constexpr float SCREEN_SIDES = 72;
/// Between two sections, from a section's kicker to its content, between
/// two cards, inside a card.
constexpr float SECTION_GAP = 36;
constexpr float HEAD_GAP    = 14;
constexpr float GAP         = 16;
constexpr float PAD_V       = 20;
constexpr float PAD_H       = 22;

/// A tab's page: a Cadran ScrollView holding a column with the page's
/// margins, which it returns.
aurore::Box* page(ScrollView* scroll);

/// A section of `page`: its kicker (an icon and its title in capitals), a
/// line saying what it shows (may be empty), then the box returned, a
/// column, for its content.
aurore::Box* section(aurore::Box* page, const char* icon, const std::string& title, const std::string& lead = "");

/// A row (or a column) whose children are GAP apart; `wrap`: on to the next
/// line when they do not fit.
aurore::Box* row(bool wrap = false, float gap = GAP);
aurore::Box* column(float gap = GAP);

/// A card of the page: Cadran's, padded, a column; `focusable` takes the
/// focus ring, `grow` shares a row with its neighbours.
aurore::Box* card(bool focusable = false, bool grow = true);

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

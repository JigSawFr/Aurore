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

#include <cstdint>
#include <string>
#include <vector>
#include <aurore.hpp>

// The Library's games: made up, every one of them (names, covers and
// figures), generated the same way at every start.
namespace library
{
enum Genre
{
    ADVENTURE,
    RACING,
    PUZZLE,
    PLATFORMER,
    STRATEGY,
    RHYTHM,
    SPACE,
    OCEAN,
    GENRE_COUNT,
};

struct Game
{
    std::string title;
    Genre genre;
    int year;
    int minutes; ///< played
    int players; ///< at most, on one console
    float completion; ///< 0 to 1
    int achievements, achieved;
    uint32_t seed; ///< its cover's
    int recent; ///< 0: played last; larger, longer ago
};

/// Every game, in the order they were added.
const std::vector<Game>& games();
/// A genre's name in the language shown, and its Cadran icon.
std::string genreName(Genre genre);
const char* genreIcon(Genre genre);
/// "12 h 40" in the language's words.
std::string playtime(int minutes);
} // namespace library

/// A game's cover, drawn with nanovg: a gradient of its colours, a motif of
/// its genre (mountains, a road at dusk, blocks, a planet…), its title in
/// the display face over a shade at the foot. Rounded at Cadran's icon
/// radius (or `setRadius`); nothing it draws leaves the rounded shape.
class CoverArt : public aurore::View
{
  public:
    CoverArt();

    void setGame(const library::Game* game);
    void setRadius(float radius) { this->radius = radius; }
    /// The title drawn on it (true by default).
    void setTitleShown(bool shown) { this->titleShown = shown; }

    void draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
        aurore::FrameContext* ctx) override;

  protected:
    aurore::AccessibilityRole getDefaultAccessibilityRole() override { return aurore::AccessibilityRole::IMAGE; }
    std::string getDefaultAccessibilityLabel() override;

  private:
    const library::Game* game = nullptr;
    float radius              = 12;
    bool titleShown           = true;
};

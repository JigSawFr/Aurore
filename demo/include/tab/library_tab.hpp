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

#include <vector>
#include <aurore.hpp>

#include "view/cover_art.hpp"

/// A cover and its genre: a cell of the library's grid, and of the rows of
/// covers under a game.
class GameCell : public aurore::RecyclingGridItem
{
  public:
    GameCell();

    void setGame(const library::Game* game);
    void prepareForReuse() override;

    static GameCell* create();

  private:
    const library::Game* game = nullptr;
    CoverArt* cover;
    aurore::Label* caption;
};

/// The Library: 302 made-up games in a grid that keeps only the cells on
/// screen (aurore::RecyclingGrid), sorted, filtered by genre, each opening
/// its page with a transition; the same games by genre in a long list.
class LibraryTab : public aurore::Box
{
  public:
    LibraryTab();

    static aurore::View* create();

    /// The games shown, in their order: indexes into library::games().
    static std::vector<size_t>& shown();

  private:
    aurore::RecyclingGrid* grid;
    aurore::Label* count;
    aurore::IconButton* sortButton;
    aurore::IconButton* genreButton;

    void refresh();
};

/// The games shown, by genre, on a screen of their own: a long list that
/// keeps only the lines on screen (aurore::RecyclerFrame), in sections.
void openGameList();

/// A game's page, pushed over the library: its cover large, what it is, what
/// to do with it, how far it was played, games like it.
void openGame(size_t index);

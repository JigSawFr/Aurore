// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// The demo on the New 3DS's top screen (aurore::TopScreen), the screen to
// look at over the touch screen: the tab shown, its name and what it is
// about, on the night with the aurora; a game's cover and its details while
// it has the focus in the Library. Made only where there is a top screen
// (the console, or its simulation).

#pragma once

#include <aurore.hpp>

#include "view/aurora_view.hpp"
#include "view/cover_art.hpp"

namespace demo::top
{

/// The main screen's: the name of the tab shown and its lead, followed as
/// the tab changes
class TabScreen : public AuroraView
{
  public:
    TabScreen();

    void draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
        aurore::FrameContext* ctx) override;

  private:
    aurore::Label* title = nullptr;
    aurore::Label* lead  = nullptr;
    int tab              = -1; // the tab its texts are for
};

/// A game's: its cover, title, genre, year and time played
class GameScreen : public AuroraView
{
  public:
    GameScreen();
    void setGame(const library::Game* game);

  private:
    CoverArt* cover       = nullptr;
    aurore::Label* title  = nullptr;
    aurore::Label* genre  = nullptr;
    aurore::Label* played = nullptr;
};

} // namespace demo::top

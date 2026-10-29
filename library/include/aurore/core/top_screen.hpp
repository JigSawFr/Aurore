// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// A screen to look at, above the app's: the New 3DS's top screen (400x240),
// over the touch screen where the app's content is. The app gives what it
// shows: a view for the focus and what is inside it (View::setTopScreenView(),
// a list's preview of the item chosen), else one for the activity on top
// (Activity::setTopScreenView()). With nothing given, TopScreenHeader: the
// title of the header that moved there (AppletFrame::setHeaderOnTopScreen()).
// The platform says the screen is there (TopScreen::setSize()), a simulation
// of it too, and draws it with TopScreen::draw() (docs/platforms.md).

#pragma once

#include <nanovg.h>

#include <aurore/core/box.hpp>
#include <aurore/core/event.hpp>
#include <aurore/core/geometry.hpp>
#include <aurore/views/label.hpp>

namespace aurore
{

class TopScreen
{
  public:
    /// The screen's size, in the pixels of the app's screen (the console's
    /// on the console, the window's in a simulation that scales it); 0 x 0,
    /// the default, when there is none
    static void setSize(int width, int height);
    static bool isAvailable() { return width > 0 && height > 0; }
    static int getWidth() { return width; }
    static int getHeight() { return height; }
    /// Fired with isAvailable() when it changes
    static Event<bool>* getAvailabilityChanged() { return &availabilityChanged; }

    /// Its size in layout units: its pixels at the app's screen's scale
    /// (Application::windowScale), so that a text is as big on both
    static float getContentWidth();
    static float getContentHeight();

    /// Draws the screen of the activity on top in `rect`, in the pixels of
    /// the frame begun on `vg` (the whole frame on the console, a part of the
    /// window in a simulation)
    static void draw(NVGcontext* vg, Rect rect);

    /// What the screen shows, nullptr for TopScreenHeader: the top screen
    /// view nearest to the focus (View::setTopScreenView()), else the
    /// activity's on top
    static View* getShownView();

    /// What a header says: the title of the activity on top (its AppletFrame's),
    /// else the app's
    static std::string getTitle();

  private:
    inline static int width  = 0;
    inline static int height = 0;
    inline static Event<bool> availabilityChanged;
};

/// The top screen of an activity that gives none: the activity's title, the
/// battery and the Wi-Fi in a header, the time below
class TopScreenHeader : public Box
{
  public:
    TopScreenHeader();
    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;

  private:
    std::string shownTitle, shownTime;
    Label* title = nullptr;
    Label* time  = nullptr;
};

} // namespace aurore

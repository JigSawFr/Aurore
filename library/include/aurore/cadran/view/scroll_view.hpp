// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

// ScrollView — the scrolling frame of every Cadran screen (<ScrollView> in
// the XML, with aurore:ScrollingFrame's attributes).
//
// Aurore's default ("natural") scrolling moves a frame's worth of pixels per
// D-pad press once the next line is partly off screen, so a short press barely
// scrolls and the focus can sit on the frame itself, with no highlight; its
// "centered" mode keeps the focused line mid-screen, which hides a section's
// header and the notes under its last line. Here each press moves the focus to
// the next line and scrolls just enough to show it with the text around it
// (the header above, the notes below, the top or the end of the screen), and a
// text taller than the screen is scrolled half a screen per press before the
// focus moves on.
#pragma once

#include <aurore.hpp>

/// The scrolling frame of Cadran's screens (`<ScrollView>`): each press moves to
/// the next line and scrolls just enough to show it with its header and notes.
class ScrollView : public aurore::ScrollingFrame
{
  public:
    ScrollView();

    void draw(NVGcontext* vg, float x, float y, float width, float height,
        aurore::Style style, aurore::FrameContext* ctx) override;
    void willAppear(bool resetState) override;
    void onChildFocusGained(aurore::View* directChild, aurore::View* focusedView) override;
    aurore::View* getNextFocus(aurore::FocusDirection direction, aurore::View* currentView) override;
    aurore::View* getDefaultFocus() override;
    void onFocusLost() override;

    static aurore::View* create();

    /// The focused line shown again at the next frame, once laid out anew: a
    /// layout moved it (the rail's width changed, cadran::apply_rail).
    static void reveal_focused();

  private:
    /// A stretch of the content, from `top` to `bottom`.
    struct Span
    {
        float top, bottom;
        float height() const { return bottom - top; }
    };

    bool reveal_pending = false;
    bool parked         = false; ///< the focus waits on the frame: see step()

    bool owns(aurore::View* view) const;
    Span span_of(aurore::View* view) const;
    Span context_of(aurore::View* view) const;
    aurore::View* neighbour(aurore::FocusDirection direction) const;
    float bottom_limit();
    aurore::View* visible_line(bool first);
    void reveal(aurore::View* view, bool animated);
    bool step(aurore::FocusDirection direction);
};

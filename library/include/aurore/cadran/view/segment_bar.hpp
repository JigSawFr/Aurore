// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

// SegmentBar — a bar for a small batch: one segment an item, side by side,
// the well while it waits, teal once done, coral when it failed. Each segment
// eases into its colour. In place of the bar of a progress dialog
// (cadran::progress_body), as high.
#pragma once

#include <vector>
#include <aurore.hpp>

class SegmentBar : public aurore::View
{
  public:
    enum class State
    {
        WAITING,
        DONE,
        FAILED
    };

    explicit SegmentBar(size_t count);
    void set(size_t index, State state);

    void draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
        aurore::FrameContext* ctx) override;

    /// Up to this many items a batch shows its segments; past it, the bar.
    static constexpr size_t MAX = 20;

  private:
    std::vector<State> states;
    std::vector<aurore::Time> since; ///< when each segment changed (µs)
};

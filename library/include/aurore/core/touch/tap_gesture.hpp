// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/gesture.hpp>

namespace aurore
{

struct TapGestureConfig
{
    bool highlightOnSelect = true;
    Sound unsureSound      = SOUND_FOCUS_CHANGE;
    Sound failedSound      = SOUND_TOUCH_UNFOCUS;
    Sound endSound         = SOUND_CLICK;

    TapGestureConfig() { }
    TapGestureConfig(bool highlightOnSelect, Sound unsureSound, Sound failedSound, Sound endSound)
    {
        this->highlightOnSelect = highlightOnSelect;
        this->unsureSound       = unsureSound;
        this->failedSound       = failedSound;
        this->endSound          = endSound;
    }
};

struct TapGestureStatus
{
    GestureState state; // Gesture state
    Point position; // Current position
};
typedef Event<TapGestureStatus, Sound*> TapGestureEvent;

// Tap recognizer
// UNSURE: while touch moves inside of View bounds
// START: unsupported
// MOVE: unsupported
// END: touch released inside View's bounds
// FAILED: touch moved outside View's bounds
class TapGestureRecognizer : public GestureRecognizer
{
  public:
    // Simple ctor which uses View's primary action as response which will be called only on recognizer state END.
    TapGestureRecognizer(View* view, TapGestureConfig config = TapGestureConfig());

    // Simple ctor with custom response which will be called only on recognizer state END.
    TapGestureRecognizer(View* view, std::function<void()> respond, TapGestureConfig config = TapGestureConfig());

    // Complex ctor with fully controllable response.
    TapGestureRecognizer(TapGestureEvent::Callback respond);

    GestureState recognitionLoop(TouchState touch, MouseState mouse, View* view, Sound* soundToPlay) override;

    // Get current state of recognizer
    TapGestureStatus getCurrentStatus();

    // Get tap gesture event
    TapGestureEvent getTapGestureEvent() const { return tapEvent; }

    void setForceRecognision(bool forceRecognision)
    {
        this->forceRecognision = forceRecognision;
    }

  private:
    TapGestureEvent tapEvent;
    Point position {};
    GestureState lastState {};
    bool forceRecognision {};
};

} // namespace aurore

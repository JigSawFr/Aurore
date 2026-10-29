// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/event.hpp>
#include <aurore/core/gesture.hpp>

namespace aurore
{

// Contains info about acceleration on pan ends
struct PanAcceleration
{
    // distances in pixels
    Point distance;

    // times to cover the distance
    Point time;
};

// Current status of gesture
struct PanGestureStatus
{
    GestureState state; // Gesture state
    Point position; // Current position
    Point startPosition; // Start X position
    Point delta; // Difference between current and previous positions by X
    bool deltaOnly = false; // If true, current state will contain delta values ONLY

    // Acceleration info, NOT NULL ONLY from
    // gesture callback and when current state is END
    PanAcceleration acceleration;
};

typedef Event<PanGestureStatus, Sound*> PanGestureEvent;

// Axis of pan recognition start conditions
enum class PanAxis
{
    HORIZONTAL, // Triggers only on horizontal coordinate changes
    VERTICAL, // Triggers only on vertical coordinate changes
    ANY, // Any movement allowed
};

// Pan recognizer
// UNSURE: while touch not moved enough to recognize it as pan
// START: gesture has been recognized
// MOVE: gesture in process
// END: finger released, acceleration will be calculated
// FAILED: unsupported
class PanGestureRecognizer : public GestureRecognizer
{
  public:
    PanGestureRecognizer(PanGestureEvent::Callback respond, PanAxis axis);
    GestureState recognitionLoop(TouchState touch, MouseState mouse, View* view, Sound* soundToPlay) override;

    // Get pan gesture axis
    PanAxis getAxis() const { return this->axis; }

    // Get current state of recognizer
    PanGestureStatus getCurrentStatus();

    // Get pan gesture event
    PanGestureEvent getPanGestureEvent() const { return panEvent; }

    static inline float panFactor { 1.0f };

  private:
    int lastFingerId = 0;
    PanGestureEvent panEvent;
    Point position;
    Point startPosition;
    Point delta;
    PanAxis axis;
    std::vector<Point> posHistory;
    GestureState lastState = GestureState::FAILED;
};

} // namespace aurore

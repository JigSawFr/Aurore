// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

// Inspector — an overlay over the screen showing the view tree, the focus
// chain, the yoga box and the theme values of a view, and the frame's timings.
#pragma once

#include <nanovg.h>

#include <string>
#include <utility>
#include <vector>

namespace aurore
{

class View;
class FrameContext;
class InputManager;
struct ControllerState;

/**
 * The built-in inspector, turned on with Application::setInspectorEnabled()
 * (docs/inspector.md). Closed, it only watches for its toggle: F12 on a
 * keyboard, or both sticks pressed in together (L3 + R3) on a controller.
 * Each press goes one step further:
 *
 *  - live: the panel follows the focus, the app keeps every input;
 *  - browse: the inspector takes the controller (and the keys mapped to it)
 *    to walk the tree, the app gets none of it;
 *  - closed again.
 *
 * The rest of this class is what the core calls; an app only needs
 * Application::setInspectorEnabled().
 */
class Inspector
{
  public:
    enum class Mode
    {
        CLOSED,
        LIVE,
        BROWSE,
    };

    static void setEnabled(bool enabled);
    static bool isEnabled() { return enabled; }

    static void setMode(Mode mode);
    static Mode getMode() { return mode; }
    static bool isOpen() { return mode != Mode::CLOSED; }

    /**
     * Reads the toggle and, when browsing, the controller, whose buttons it
     * then clears from `state` so that the app does not see them.
     */
    static void processInput(InputManager* input, ControllerState* state);

    // In Application::frame(), when open. The timings shown are the
    // Profiler's, which measures while the inspector is open.
    static void beginFrame();
    static void draw(FrameContext* ctx);

    static void forgetView(View* view);

    // Hooks of View, ThemeValues and StyleValues. `recordTarget` is the
    // selected view while the inspector is open (nullptr otherwise), and
    // `recording` is true while that view itself (not its children) draws.
    inline static View* recordTarget = nullptr;
    inline static bool recording     = false;
    static void recordColor(const std::string& name, NVGcolor color);
    static void recordMetric(const std::string& name, float value);

  private:
    inline static bool enabled = false;
    inline static Mode mode    = Mode::CLOSED;
};

} // namespace aurore

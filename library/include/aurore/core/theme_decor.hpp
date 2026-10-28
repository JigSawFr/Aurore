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

#include <nanovg.h>

#include <aurore/core/theme.hpp>
#include <aurore/core/time.hpp>

namespace aurore
{

/**
 * What a theme decorates the screens with (docs/themes.md, Decorations),
 * in the variant in use: a picture over the surface of an area (the
 * header, the sidebar, the dialogs: View::setThemeArea), strips along the
 * top and bottom edges of the screen, particles. Application draws them;
 * without them nothing is drawn and nothing changes.
 *
 * The particles move only when motion is allowed
 * (Application::getReduceMotion() false: neither the app's reduced motion
 * nor, with Cadran, its lighter or reduced visual effects or still
 * screenshots), else nothing of them is drawn. They move at PARTICLE_FPS
 * (with redraw on demand, the frames they ask for), and stop after
 * IDLE_SECONDS without input (they fade out, and ask for no frame until
 * the next input). They never take the focus, an input or a touch, and are
 * no view: the layout does not know them.
 *
 * Only the UI thread calls these.
 */
class ThemeDecorations
{
  public:
    // The particles at most (a density of 1)
    static constexpr int MAX_PARTICLES = 60;
    // Their rate: they move at most this many times a second
    static constexpr int PARTICLE_FPS = 30;
    // Without input this long, they fade out and stop
    static constexpr int IDLE_SECONDS = 60;

    /**
     * The theme or the variant changed (Application): the pictures of
     * `variant` loaded (those of the theme before stay until they are
     * ready), its particles set up from its colours.
     */
    static void update(ThemeVariant variant);

    /**
     * The picture of `area` (HEADER, SIDEBAR, DIALOG) over x, y, width,
     * height, with corners of `radius`; nothing without one.
     */
    static void drawBackdrop(ThemeArea area, NVGcontext* vg, float x, float y, float width, float height, float radius = 0.0f);

    /**
     * The strips (decor.top, decor.bottom) along the edges of the screen,
     * the part of them over x, y, width, height.
     */
    static void drawStrips(NVGcontext* vg, float x, float y, float width, float height);

    /**
     * The particles of `layer` over the screen (width, height: the
     * layout's), where they are now; nothing while they do not run.
     */
    static void drawParticles(NVGcontext* vg, ParticleLayer layer, float width, float height);

    /**
     * Once per iteration of the main loop, after the input: whether the
     * particles moved since the frame last drawn (a frame is due, with
     * redraw on demand).
     */
    static bool step(Time now);

    /**
     * Whether the particles are shown and moving now: the theme has some,
     * motion is allowed, there was input in the last IDLE_SECONDS (and the
     * time they take to fade out).
     */
    static bool particlesRunning();

    /**
     * How many particles the theme in use has (0 without), running or not.
     */
    static int particleCount();

    /**
     * Whether the theme in use has a strip or a backdrop picture loaded.
     */
    static bool hasPicture(ThemeArea area);

    /**
     * Lets go of every picture (Application::exit).
     */
    static void reset();
};

} // namespace aurore

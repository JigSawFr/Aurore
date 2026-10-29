// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <nanovg.h>

#include <cmath>
#include <vector>

// A VideoContext is responsible for providing a nanovg context for the app
// (so by extension it manages all the graphics state as well as the window / context).
// The VideoContext implementation must also provide the nanovg implementation. As such, there
// can only be one VideoContext linked at any time in the binary.
// Context creation and teardown can be done in the constructor and destructor.
class VideoContext
{
  public:
    virtual ~VideoContext() = default;

    /**
     * Called at the beginning of every frame to clear the window
     */
    virtual void clear(NVGcolor color) = 0;

    /**
     * Called at the beginning of every frame to begin it.
     */
    virtual void beginFrame() = 0;

    /**
     * Called at the end of every frame to end it (swap buffers...).
     */
    virtual void endFrame() = 0;

    /**
     * Can be called by the application to reset the graphics
     * state, in case there is a need to use the graphics API
     * directly (for instance direct OpenGL calls).
     */
    virtual void resetState() = 0;

    /*
     * scale factor between window size and drawable size
     */
    virtual double getScaleFactor() = 0;

    /*
     * Full screen window at primary monitor
     */
    virtual void fullScreen(bool fs) { }

    virtual NVGcontext* getNVGContext() = 0;

    /**
     * Reads back the frame drawn so far (before endFrame() presents it):
     * top-down rows of opaque RGBA pixels, at the drawable's size. Returns
     * false where the driver cannot (only the OpenGL ones do for now).
     */
    virtual bool readPixels(std::vector<unsigned char>* rgba, int* width, int* height) { return false; }

    virtual void setSwapInterval(int interval) = 0;

    virtual int getCurrentMonitorIndex() { return 0; }

    static inline bool FULLSCREEN = false;

    // The position and size of the window in the last non-full-screen mode
    static inline float posX = NAN, posY = NAN;
    static inline uint32_t sizeW = 0, sizeH = 0;

    static inline int monitorIndex = 0;

    static inline int swapInterval = 1;
};

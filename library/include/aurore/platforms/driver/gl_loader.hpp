// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

// The OpenGL functions of the current context, loaded with glad. Include it
// after <glad/gl.h>.

#include <string>
#include <aurore/core/logger.hpp>

namespace aurore
{

// Loads the functions of the context current on this thread with `load`
// (glfwGetProcAddress, SDL_GL_GetProcAddress, eglGetProcAddress), as the
// driver built in draws: OpenGL ES with the GLES2 and GLES3 drivers, OpenGL
// otherwise. False, and the reason in the log, when the context cannot be
// loaded or is older than the driver needs (2.0 for GL2 and GLES2, 3.2 for
// GL3, ES 3.0 for GLES3). `who` prefixes the log lines.
inline bool loadGL(GLADloadfunc load, const std::string& who)
{
#if defined(USE_GLES2) || defined(USE_GLES3)
    const int version = gladLoadGLES2(load);
    const char* api   = "OpenGL ES";
#if defined(USE_GLES3)
    const int needed = GLAD_MAKE_VERSION(3, 0);
#else
    const int needed = GLAD_MAKE_VERSION(2, 0);
#endif
#else
    const int version = gladLoadGL(load);
    const char* api   = "OpenGL";
#if defined(USE_GL2)
    const int needed = GLAD_MAKE_VERSION(2, 0);
#else
    const int needed = GLAD_MAKE_VERSION(3, 2);
#endif
#endif
    if (version == 0)
    {
        Logger::error("{}: no {} functions in the current context", who, api);
        return false;
    }
    if (version < needed)
    {
        Logger::error("{}: {} {}.{}, {}.{} needed", who, api, GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version),
            GLAD_VERSION_MAJOR(needed), GLAD_VERSION_MINOR(needed));
        return false;
    }
    Logger::info("{}: {} {}.{} loaded", who, api, GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
    return true;
}

} // namespace aurore

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

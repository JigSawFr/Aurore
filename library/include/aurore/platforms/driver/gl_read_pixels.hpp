// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

#pragma once

// VideoContext::readPixels() for the OpenGL drivers. Include it after the
// driver's GL header (glad, GLES2...).

#include <cstring>
#include <vector>

namespace aurore
{

// The framebuffer bound for reading, `width` x `height` from its corner:
// GL's rows go bottom-up, the result's top-down; alpha is set opaque (what
// the screen shows).
inline bool glReadPixelsTopDown(int width, int height, std::vector<unsigned char>* rgba)
{
    if (width <= 0 || height <= 0)
        return false;
    const size_t stride = (size_t)width * 4;
    std::vector<unsigned char> raw(stride * height);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, raw.data());
    if (glGetError() != GL_NO_ERROR)
        return false;
    rgba->resize(raw.size());
    for (int y = 0; y < height; y++)
        std::memcpy(rgba->data() + (size_t)y * stride, raw.data() + (size_t)(height - 1 - y) * stride, stride);
    for (size_t i = 3; i < rgba->size(); i += 4)
        (*rgba)[i] = 255;
    return true;
}

} // namespace aurore

// nanovg's citro3d renderer: the edges' antialiasing, without a fragment
// shader.
//
// Copyright (c) 2026 JigSawFr
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
// With edge antialiasing, nanovg gives each vertex of a fill's fringe or of
// a stroke its place across the edge, `u` (0 and 1 the two sides), and
// nanovg's GL shader turns it into a coverage per pixel:
//
//     min(1, (1 - |2u - 1|) * strokeMult)
//
// strokeMult being 1 for a fill and (strokeWidth / 2 + fringe / 2) / fringe
// for a stroke. The PICA200 has no fragment shader: the coverage is read from
// a texture instead, its columns `u` and its rows strokeMult, a call's row
// chosen once. Pure arithmetic, no citro3d: the desktop tests check it
// against the formula (tests/demo_tests.cpp, citro3d_coverage).

#ifndef NANOVG_CITRO3D_AA_H
#define NANOVG_CITRO3D_AA_H

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace nvgc3d::aa
{

// The coverage texture: 1024 places across an edge (the hardware's widest),
// 64 strokeMults from 1 to 64 a constant ratio apart. Off nanovg's own by
// at most 3.3 % of full coverage up to a strokeMult of 16 (a stroke 31
// fringes wide), 6 % up to 64; a wider stroke takes row 0, its fringe
// a little wider than nanovg's.
constexpr int WIDTH  = 1024;
constexpr int ROWS   = 64;
constexpr float MULT = 64.0f; // the widest row's strokeMult

// nanovg GL's coverage
inline float coverage(float u, float strokeMult)
{
    return std::clamp((1.0f - std::fabs(2.0f * u - 1.0f)) * strokeMult, 0.0f, 1.0f);
}

// The strokeMult of a row: MULT (row 0) down to 1 (the last row, a fill or
// a hairline)
inline float rowMult(int row)
{
    return std::pow(MULT, (float)(ROWS - 1 - row) / (float)(ROWS - 1));
}

// The row for a strokeMult, the nearest
inline int rowOf(float strokeMult)
{
    float m = std::clamp(strokeMult, 1.0f, MULT);
    int row = (int)std::lround((ROWS - 1) * (1.0f - std::log(m) / std::log(MULT)));
    return std::clamp(row, 0, ROWS - 1);
}

// The texture coordinate of a row's middle
inline float rowT(int row)
{
    return ((float)row + 0.5f) / (float)ROWS;
}

// A texel: the coverage at the middle of column x, in row `row`
inline uint8_t texel(int x, int row)
{
    float u = ((float)x + 0.5f) / (float)WIDTH;
    return (uint8_t)std::lround(coverage(u, rowMult(row)) * 255.0f);
}

} // namespace nvgc3d::aa

#endif /* NANOVG_CITRO3D_AA_H */

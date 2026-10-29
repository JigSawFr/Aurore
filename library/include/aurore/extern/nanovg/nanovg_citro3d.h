//
// nanovg's citro3d renderer, for the New 3DS's PICA200.
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
// Built on nanovg_gl.h's structure: the frame's calls are recorded, then
// drawn at nvgEndFrame. The PICA200 has no fragment shader, so what the GL
// shader computes per pixel is done another way:
//
// - the paint's coordinates (image, gradient) are an affine function of the
//   position, computed per vertex by the vertex shader (exact);
// - colours are the texture combiners' constants, premultiplied;
// - a linear gradient reads a shared ramp texture, mixed between its two
//   colours by the combiners (exact to 8 bits);
// - a radial or box gradient reads a small alpha texture of its distance
//   field, made on the CPU and kept in a cache (close, not exact);
// - nonzero fills use the 8-bit stencil in two passes (one per face, the
//   PICA200 has a single stencil operation for both), then a cover pass;
// - the scissor is the hardware's, the box around nanovg's (Aurore only
//   clips to axis-aligned rectangles).
//
// Not yet: edge antialiasing (the context is made without it), stencil
// strokes, mipmaps. Textures are padded to powers of two, at most 1024; a
// larger image is scaled down when uploaded.
//
// The caller owns citro3d (C3D_Init, the render target, C3D_FrameBegin and
// C3D_FrameEnd): nvgEndFrame draws on the target bound with C3D_FrameDrawOn.
// Call nvgC3DFrameBegin after C3D_FrameBegin: the GPU is then done with the
// previous frame, and the textures deleted since are freed.

#ifndef NANOVG_CITRO3D_H
#define NANOVG_CITRO3D_H

#include "nanovg.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Makes the context: flags are nanovg_gl.h's (none is read yet)
    NVGcontext* nvgCreateC3D(int flags);
    void nvgDeleteC3D(NVGcontext* ctx);

    // Once per frame, after C3D_FrameBegin
    void nvgC3DFrameBegin(NVGcontext* ctx);

#ifdef __cplusplus
}
#endif

#endif /* NANOVG_CITRO3D_H */

#ifdef NANOVG_CITRO3D_IMPLEMENTATION

#include <citro3d.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <list>
#include <unordered_map>
#include <vector>

// Made by library/CMakeLists.txt from nanovg_citro3d.v.pica (picasso)
#include "nanovg_citro3d_shbin.h"

namespace nvgc3d
{

constexpr int MAX_TEXTURE_SIZE = 1024;
// Box and radial gradient textures kept
constexpr size_t GRADIENT_CACHE_SIZE = 24;
constexpr int RAMP_WIDTH             = 256;

struct Texture
{
    C3D_Tex tex   = {};
    int type      = 0; // NVG_TEXTURE_ALPHA or NVG_TEXTURE_RGBA
    int flags     = 0;
    int width     = 0; // as nanovg sees it
    int height    = 0;
    int stored[2] = { 0, 0 }; // what is stored (smaller than width x height when scaled down)
};

enum class PaintKind
{
    SOLID,
    IMAGE,
    IMAGE_ALPHA,
    GRADIENT,
};

struct Paint
{
    PaintKind kind = PaintKind::SOLID;
    uint32_t inner = 0; // premultiplied, as the combiners take it (R in the low byte)
    uint32_t outer = 0;
    // A copy: citro3d reads it when drawing, and a texture deleted during
    // the frame is only freed at the next one
    C3D_Tex tex     = {};
    float row[2][4] = {};
    float off[2]    = {};
};

enum class CallType
{
    FILL,
    CONVEX_FILL,
    STROKE,
    TRIANGLES,
};

struct Call
{
    CallType type;
    Paint paint;
    int pathOffset;
    int pathCount;
    int triangleOffset; // the cover quad for FILL, the vertices for TRIANGLES
    int triangleCount;
    GPU_BLENDFACTOR blend[4]; // src RGB, dst RGB, src alpha, dst alpha
    bool scissor;
    uint32_t scissorBox[4]; // left, top, right, bottom in the framebuffer's (portrait) coordinates
};

struct Path
{
    int fillOffset;
    int fillCount;
    int strokeOffset;
    int strokeCount;
};

struct GradientKey
{
    int ex, ey, radius, feather; // in eighths of a unit
    bool operator==(const GradientKey& o) const
    {
        return ex == o.ex && ey == o.ey && radius == o.radius && feather == o.feather;
    }
};

struct GradientKeyHash
{
    size_t operator()(const GradientKey& k) const
    {
        size_t h = (size_t)k.ex * 73856093u;
        h ^= (size_t)k.ey * 19349663u;
        h ^= (size_t)k.radius * 83492791u;
        h ^= (size_t)k.feather * 2654435761u;
        return h;
    }
};

struct Gradient
{
    GradientKey key;
    C3D_Tex tex;
};

struct Context
{
    DVLB_s* dvlb = nullptr;
    shaderProgram_s program;
    int projLocation   = -1;
    int texRowLocation = -1;
    int texOffLocation = -1;

    std::unordered_map<int, Texture> textures;
    int nextTexture = 1;
    // Deleted while the GPU may still read them: freed at the next frame
    std::vector<C3D_Tex> pendingDeletes;

    C3D_Tex ramp = {};
    std::list<Gradient> gradients; // most recently used first
    std::unordered_map<GradientKey, std::list<Gradient>::iterator, GradientKeyHash> gradientIndex;

    float viewWidth  = 0;
    float viewHeight = 0;

    std::vector<Call> calls;
    std::vector<Path> paths;
    std::vector<NVGvertex> verts;

    NVGvertex* linearVerts = nullptr;
    size_t linearCapacity  = 0;
};

// PICA200 textures are 8x8 tiles in rows, Morton order inside a tile, the
// first row of tiles at the bottom
inline size_t tiledOffset(unsigned x, unsigned y, unsigned width)
{
    size_t tile = ((y >> 3) * (width >> 3) + (x >> 3)) << 6;
    return tile | (x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3);
}

inline int nextPow2(int v)
{
    int p = 8;
    while (p < v)
        p <<= 1;
    return p;
}

inline uint8_t toByte(float v)
{
    return (uint8_t)std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f);
}

// The combiners' constant: premultiplied, R in the low byte
inline uint32_t premulColor(NVGcolor c)
{
    return (uint32_t)toByte(c.r * c.a) | ((uint32_t)toByte(c.g * c.a) << 8) | ((uint32_t)toByte(c.b * c.a) << 16)
        | ((uint32_t)toByte(c.a) << 24);
}

inline GPU_BLENDFACTOR blendFactor(int factor)
{
    switch (factor)
    {
        case NVG_ZERO:
            return GPU_ZERO;
        case NVG_ONE:
            return GPU_ONE;
        case NVG_SRC_COLOR:
            return GPU_SRC_COLOR;
        case NVG_ONE_MINUS_SRC_COLOR:
            return GPU_ONE_MINUS_SRC_COLOR;
        case NVG_DST_COLOR:
            return GPU_DST_COLOR;
        case NVG_ONE_MINUS_DST_COLOR:
            return GPU_ONE_MINUS_DST_COLOR;
        case NVG_SRC_ALPHA:
            return GPU_SRC_ALPHA;
        case NVG_ONE_MINUS_SRC_ALPHA:
            return GPU_ONE_MINUS_SRC_ALPHA;
        case NVG_DST_ALPHA:
            return GPU_DST_ALPHA;
        case NVG_ONE_MINUS_DST_ALPHA:
            return GPU_ONE_MINUS_DST_ALPHA;
        case NVG_SRC_ALPHA_SATURATE:
            return GPU_SRC_ALPHA_SATURATE;
        default:
            return GPU_ONE;
    }
}

// Writes the rectangle of an image into its texture, from the image's rows
// (`data` is the whole image, `width` wide). Pixels are premultiplied unless
// they already are; past the right and bottom edges, the edge is repeated
// into the padding, so that linear filtering stops at the image.
inline void upload(Texture& t, int x, int y, int w, int h, const unsigned char* data)
{
    auto* out    = static_cast<uint8_t*>(t.tex.data);
    unsigned tw  = t.tex.width;
    unsigned th  = t.tex.height;
    int sw       = t.stored[0];
    int sh       = t.stored[1];
    bool scaled  = sw != t.width || sh != t.height;
    bool premult = t.flags & NVG_IMAGE_PREMULTIPLIED;
    if (scaled)
    {
        // Scaled down: the whole image again
        x = 0;
        y = 0;
        w = sw;
        h = sh;
    }
    int endX = x + w >= sw ? (int)tw : x + w;
    int endY = y + h >= sh ? (int)th : y + h;
    for (int ty = y; ty < endY; ty++)
    {
        int sy = std::min(ty, sh - 1);
        if (scaled)
            sy = (int)((int64_t)sy * t.height / sh);
        for (int tx = x; tx < endX; tx++)
        {
            int sx = std::min(tx, sw - 1);
            if (scaled)
                sx = (int)((int64_t)sx * t.width / sw);
            size_t o = tiledOffset(tx, th - 1 - ty, tw);
            if (t.type == NVG_TEXTURE_ALPHA)
                out[o] = data[(size_t)sy * t.width + sx];
            else
            {
                const unsigned char* p = data + ((size_t)sy * t.width + sx) * 4;
                unsigned a             = p[3];
                unsigned r = p[0], g = p[1], b = p[2];
                if (!premult)
                {
                    r = (r * a + 127) / 255;
                    g = (g * a + 127) / 255;
                    b = (b * a + 127) / 255;
                }
                // GPU_RGBA8 is ABGR in memory
                out[o * 4 + 0] = (uint8_t)a;
                out[o * 4 + 1] = (uint8_t)b;
                out[o * 4 + 2] = (uint8_t)g;
                out[o * 4 + 3] = (uint8_t)r;
            }
        }
    }
    C3D_TexFlush(&t.tex);
}

inline int renderCreateTexture(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data)
{
    auto* ctx = static_cast<Context*>(uptr);
    if (w <= 0 || h <= 0)
        return 0;
    Texture t;
    t.type   = type;
    t.flags  = imageFlags;
    t.width  = w;
    t.height = h;
    // Scaled down to fit, keeping the proportions
    float scale = std::min(1.0f, (float)MAX_TEXTURE_SIZE / (float)std::max(w, h));
    t.stored[0] = std::max(1, (int)(w * scale));
    t.stored[1] = std::max(1, (int)(h * scale));
    if (scale < 1.0f)
        std::fprintf(stderr, "nanovg citro3d: %dx%d image scaled down to %dx%d\n", w, h, t.stored[0], t.stored[1]);
    if (!C3D_TexInit(&t.tex, nextPow2(t.stored[0]), nextPow2(t.stored[1]), type == NVG_TEXTURE_ALPHA ? GPU_A8 : GPU_RGBA8))
    {
        std::fprintf(stderr, "nanovg citro3d: no memory for a %dx%d texture\n", w, h);
        return 0;
    }
    std::memset(t.tex.data, 0, t.tex.size);
    GPU_TEXTURE_FILTER_PARAM filter = (imageFlags & NVG_IMAGE_NEAREST) ? GPU_NEAREST : GPU_LINEAR;
    C3D_TexSetFilter(&t.tex, filter, filter);
    // Repeating needs the image to fill its texture
    bool fills = t.stored[0] == t.tex.width && t.stored[1] == t.tex.height && scale == 1.0f;
    C3D_TexSetWrap(&t.tex, (imageFlags & NVG_IMAGE_REPEATX) && fills ? GPU_REPEAT : GPU_CLAMP_TO_EDGE,
        (imageFlags & NVG_IMAGE_REPEATY) && fills ? GPU_REPEAT : GPU_CLAMP_TO_EDGE);
    if (data)
        upload(t, 0, 0, w, h, data);
    else
        C3D_TexFlush(&t.tex);
    int id            = ctx->nextTexture++;
    ctx->textures[id] = t;
    return id;
}

inline int renderDeleteTexture(void* uptr, int image)
{
    auto* ctx = static_cast<Context*>(uptr);
    auto it   = ctx->textures.find(image);
    if (it == ctx->textures.end())
        return 0;
    ctx->pendingDeletes.push_back(it->second.tex);
    ctx->textures.erase(it);
    return 1;
}

inline int renderUpdateTexture(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data)
{
    auto* ctx = static_cast<Context*>(uptr);
    auto it   = ctx->textures.find(image);
    if (it == ctx->textures.end() || !data)
        return 0;
    upload(it->second, x, y, w, h, data);
    return 1;
}

inline int renderGetTextureSize(void* uptr, int image, int* w, int* h)
{
    auto* ctx = static_cast<Context*>(uptr);
    auto it   = ctx->textures.find(image);
    if (it == ctx->textures.end())
        return 0;
    *w = it->second.width;
    *h = it->second.height;
    return 1;
}

inline void renderViewport(void* uptr, float width, float height, float)
{
    auto* ctx       = static_cast<Context*>(uptr);
    ctx->viewWidth  = width;
    ctx->viewHeight = height;
}

inline void resetFrame(Context* ctx)
{
    ctx->calls.clear();
    ctx->paths.clear();
    ctx->verts.clear();
}

inline void renderCancel(void* uptr)
{
    resetFrame(static_cast<Context*>(uptr));
}

// nanovg's GL shader's distance to a rounded rectangle
inline float sdRoundRect(float px, float py, float ex, float ey, float r)
{
    float dx = std::fabs(px) - (ex - r);
    float dy = std::fabs(py) - (ey - r);
    float mx = std::max(dx, 0.0f);
    float my = std::max(dy, 0.0f);
    return std::min(std::max(dx, dy), 0.0f) + std::sqrt(mx * mx + my * my) - r;
}

// The alpha texture of a box (or radial) gradient: how far it goes from the
// inner colour to the outer one, over the box grown by the feather. A cache
// keeps the recent ones: shadows repeat from frame to frame.
inline C3D_Tex* gradientTexture(Context* ctx, float ex, float ey, float radius, float feather)
{
    GradientKey key { (int)std::lround(ex * 8), (int)std::lround(ey * 8), (int)std::lround(radius * 8),
        (int)std::lround(feather * 8) };
    auto found = ctx->gradientIndex.find(key);
    if (found != ctx->gradientIndex.end())
    {
        ctx->gradients.splice(ctx->gradients.begin(), ctx->gradients, found->second);
        return &found->second->tex;
    }

    // A few texels across the feather, at most 256
    float spanX = ex + feather;
    float spanY = ey + feather;
    int w       = std::min(256, nextPow2((int)std::ceil(spanX * 2 / std::max(feather, 0.5f) * 4)));
    int h       = std::min(256, nextPow2((int)std::ceil(spanY * 2 / std::max(feather, 0.5f) * 4)));

    if (ctx->gradients.size() >= GRADIENT_CACHE_SIZE)
    {
        auto& last = ctx->gradients.back();
        ctx->pendingDeletes.push_back(last.tex);
        ctx->gradientIndex.erase(last.key);
        ctx->gradients.pop_back();
    }
    Gradient g;
    g.key = key;
    if (!C3D_TexInit(&g.tex, w, h, GPU_A8))
        return nullptr;
    C3D_TexSetFilter(&g.tex, GPU_LINEAR, GPU_LINEAR);
    C3D_TexSetWrap(&g.tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    auto* out = static_cast<uint8_t*>(g.tex.data);
    float r   = std::min(radius, std::min(ex, ey));
    for (int y = 0; y < h; y++)
    {
        // Symmetric in both axes: the texture's orientation does not matter
        float py = -spanY + (y + 0.5f) * 2 * spanY / h;
        for (int x = 0; x < w; x++)
        {
            float px                  = -spanX + (x + 0.5f) * 2 * spanX / w;
            float d                   = (sdRoundRect(px, py, ex, ey, r) + feather * 0.5f) / feather;
            out[tiledOffset(x, y, w)] = toByte(d);
        }
    }
    C3D_TexFlush(&g.tex);
    ctx->gradients.push_front(g);
    ctx->gradientIndex[key] = ctx->gradients.begin();
    return &ctx->gradients.front().tex;
}

// What the combiners and the vertex shader need for a paint. `triangles`:
// the vertices carry the texture coordinates (text).
inline Paint convertPaint(Context* ctx, const NVGpaint* p, bool triangles)
{
    Paint out;
    out.inner = premulColor(p->innerColor);
    out.outer = premulColor(p->outerColor);

    float inv[6];
    nvgTransformInverse(inv, p->xform);

    if (p->image)
    {
        auto it = ctx->textures.find(p->image);
        if (it == ctx->textures.end())
            return out; // drawn in its colour
        Texture& t = it->second;
        out.kind   = t.type == NVG_TEXTURE_ALPHA ? PaintKind::IMAGE_ALPHA : PaintKind::IMAGE;
        out.tex    = t.tex;
        float sx   = (float)t.stored[0] / t.tex.width;
        float sy   = (float)t.stored[1] / t.tex.height;
        if (triangles)
        {
            out.row[0][2] = sx;
            out.row[1][3] = sy;
            return out;
        }
        // The image's [0, 1] coordinates: the inverse transform over the extent
        float ix      = sx / p->extent[0];
        float iy      = sy / p->extent[1];
        out.row[0][0] = inv[0] * ix;
        out.row[0][1] = inv[2] * ix;
        out.off[0]    = inv[4] * ix;
        out.row[1][0] = inv[1] * iy;
        out.row[1][1] = inv[3] * iy;
        out.off[1]    = inv[5] * iy;
        if (t.flags & NVG_IMAGE_FLIPY)
        {
            out.row[1][0] = -out.row[1][0];
            out.row[1][1] = -out.row[1][1];
            out.off[1]    = sy - out.off[1];
        }
        return out;
    }

    if (std::memcmp(&p->innerColor, &p->outerColor, sizeof(NVGcolor)) == 0)
        return out; // a colour

    float feather = std::max(p->feather, 1e-3f);
    out.kind      = PaintKind::GRADIENT;
    if (p->radius == 0 && p->extent[0] >= 1e4f)
    {
        // nvgLinearGradient: the ramp, along the transformed y
        out.tex       = ctx->ramp;
        out.row[0][0] = inv[1] / feather;
        out.row[0][1] = inv[3] / feather;
        out.off[0]    = (inv[5] - p->extent[1]) / feather + 0.5f;
        out.off[1]    = 0.5f;
        return out;
    }

    // nvgBoxGradient, nvgRadialGradient: the box grown by the feather
    float ex     = p->extent[0];
    float ey     = p->extent[1];
    C3D_Tex* tex = gradientTexture(ctx, ex, ey, p->radius, feather);
    if (!tex)
    {
        out.kind = PaintKind::SOLID;
        return out;
    }
    out.tex       = *tex;
    float spanX   = 2 * (ex + feather);
    float spanY   = 2 * (ey + feather);
    out.row[0][0] = inv[0] / spanX;
    out.row[0][1] = inv[2] / spanX;
    out.off[0]    = inv[4] / spanX + 0.5f;
    out.row[1][0] = inv[1] / spanY;
    out.row[1][1] = inv[3] / spanY;
    out.off[1]    = inv[5] / spanY + 0.5f;
    return out;
}

// The scissor's box in the framebuffer: the screens are portrait, turned
// 90 degrees (x grows down the framebuffer's height, y right to left)
inline bool convertScissor(Context* ctx, const NVGscissor* s, uint32_t box[4])
{
    if (s->extent[0] < -0.5f || s->extent[1] < -0.5f)
        return false;
    float ex   = std::fabs(s->xform[0]) * s->extent[0] + std::fabs(s->xform[2]) * s->extent[1];
    float ey   = std::fabs(s->xform[1]) * s->extent[0] + std::fabs(s->xform[3]) * s->extent[1];
    float minX = std::clamp(s->xform[4] - ex, 0.0f, ctx->viewWidth);
    float maxX = std::clamp(s->xform[4] + ex, 0.0f, ctx->viewWidth);
    float minY = std::clamp(s->xform[5] - ey, 0.0f, ctx->viewHeight);
    float maxY = std::clamp(s->xform[5] + ey, 0.0f, ctx->viewHeight);
    box[0]     = (uint32_t)std::lround(ctx->viewHeight - maxY);
    box[1]     = (uint32_t)std::lround(ctx->viewWidth - maxX);
    box[2]     = (uint32_t)std::lround(ctx->viewHeight - minY);
    box[3]     = (uint32_t)std::lround(ctx->viewWidth - minX);
    return true;
}

inline Call& newCall(Context* ctx, CallType type, NVGpaint* paint, NVGcompositeOperationState op, NVGscissor* scissor,
    bool triangles)
{
    ctx->calls.push_back({});
    Call& call    = ctx->calls.back();
    call.type     = type;
    call.paint    = convertPaint(ctx, paint, triangles);
    call.blend[0] = blendFactor(op.srcRGB);
    call.blend[1] = blendFactor(op.dstRGB);
    call.blend[2] = blendFactor(op.srcAlpha);
    call.blend[3] = blendFactor(op.dstAlpha);
    call.scissor  = convertScissor(ctx, scissor, call.scissorBox);
    return call;
}

inline int addVerts(Context* ctx, const NVGvertex* v, int n)
{
    int offset = (int)ctx->verts.size();
    ctx->verts.insert(ctx->verts.end(), v, v + n);
    return offset;
}

inline void renderFill(void* uptr, NVGpaint* paint, NVGcompositeOperationState op, NVGscissor* scissor, float,
    const float* bounds, const NVGpath* paths, int npaths)
{
    auto* ctx       = static_cast<Context*>(uptr);
    bool convex     = npaths == 1 && paths[0].convex;
    Call& call      = newCall(ctx, convex ? CallType::CONVEX_FILL : CallType::FILL, paint, op, scissor, false);
    call.pathOffset = (int)ctx->paths.size();
    call.pathCount  = npaths;
    for (int i = 0; i < npaths; i++)
    {
        const NVGpath& p = paths[i];
        Path path        = {};
        if (p.nfill > 0)
        {
            path.fillOffset = addVerts(ctx, p.fill, p.nfill);
            path.fillCount  = p.nfill;
        }
        if (p.nstroke > 0)
        {
            path.strokeOffset = addVerts(ctx, p.stroke, p.nstroke);
            path.strokeCount  = p.nstroke;
        }
        ctx->paths.push_back(path);
    }
    if (!convex)
    {
        // The cover quad, as a strip
        NVGvertex quad[4] = {
            { bounds[2], bounds[3], 0.5f, 1.0f },
            { bounds[2], bounds[1], 0.5f, 1.0f },
            { bounds[0], bounds[3], 0.5f, 1.0f },
            { bounds[0], bounds[1], 0.5f, 1.0f },
        };
        call.triangleOffset = addVerts(ctx, quad, 4);
        call.triangleCount  = 4;
    }
}

inline void renderStroke(void* uptr, NVGpaint* paint, NVGcompositeOperationState op, NVGscissor* scissor, float, float,
    const NVGpath* paths, int npaths)
{
    auto* ctx       = static_cast<Context*>(uptr);
    Call& call      = newCall(ctx, CallType::STROKE, paint, op, scissor, false);
    call.pathOffset = (int)ctx->paths.size();
    call.pathCount  = npaths;
    for (int i = 0; i < npaths; i++)
    {
        Path path = {};
        if (paths[i].nstroke > 0)
        {
            path.strokeOffset = addVerts(ctx, paths[i].stroke, paths[i].nstroke);
            path.strokeCount  = paths[i].nstroke;
        }
        ctx->paths.push_back(path);
    }
}

inline void renderTriangles(void* uptr, NVGpaint* paint, NVGcompositeOperationState op, NVGscissor* scissor,
    const NVGvertex* verts, int nverts, float)
{
    auto* ctx           = static_cast<Context*>(uptr);
    Call& call          = newCall(ctx, CallType::TRIANGLES, paint, op, scissor, true);
    call.triangleOffset = addVerts(ctx, verts, nverts);
    call.triangleCount  = nverts;
}

inline void setPaint(Context* ctx, Paint& p)
{
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnv* mix = C3D_GetTexEnv(1);
    C3D_TexEnvInit(mix);
    switch (p.kind)
    {
        case PaintKind::SOLID:
            C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT);
            C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
            break;
        case PaintKind::IMAGE:
            C3D_TexBind(0, &p.tex);
            C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_CONSTANT);
            C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);
            break;
        case PaintKind::IMAGE_ALPHA:
            // The colour times the texture's alpha
            C3D_TexBind(0, &p.tex);
            C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT, GPU_TEXTURE0);
            C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_ALPHA);
            C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);
            break;
        case PaintKind::GRADIENT:
            // inner, then outer * t + inner * (1 - t), t the texture's alpha
            C3D_TexBind(0, &p.tex);
            C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT);
            C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
            C3D_TexEnvSrc(mix, C3D_Both, GPU_CONSTANT, GPU_PREVIOUS, GPU_TEXTURE0);
            C3D_TexEnvOpRgb(mix, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_ALPHA);
            C3D_TexEnvFunc(mix, C3D_Both, GPU_INTERPOLATE);
            C3D_TexEnvColor(mix, p.outer);
            break;
    }
    C3D_TexEnvColor(env, p.inner);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, ctx->texRowLocation, p.row[0][0], p.row[0][1], p.row[0][2], p.row[0][3]);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, ctx->texRowLocation + 1, p.row[1][0], p.row[1][1], p.row[1][2], p.row[1][3]);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, ctx->texOffLocation, p.off[0], p.off[1], 0.0f, 0.0f);
}

inline void drawFans(Context* ctx, const Call& call)
{
    for (int i = 0; i < call.pathCount; i++)
    {
        const Path& p = ctx->paths[call.pathOffset + i];
        if (p.fillCount >= 3)
            C3D_DrawArrays(GPU_TRIANGLE_FAN, p.fillOffset, p.fillCount);
    }
}

inline void drawStrips(Context* ctx, const Call& call)
{
    for (int i = 0; i < call.pathCount; i++)
    {
        const Path& p = ctx->paths[call.pathOffset + i];
        if (p.strokeCount >= 3)
            C3D_DrawArrays(GPU_TRIANGLE_STRIP, p.strokeOffset, p.strokeCount);
    }
}

inline void renderFlush(void* uptr)
{
    auto* ctx = static_cast<Context*>(uptr);
    if (ctx->calls.empty() || ctx->verts.empty())
    {
        resetFrame(ctx);
        return;
    }

    // The vertices in linear memory, where the GPU reads them (citro3d
    // flushes the linear heap from the cache when the frame ends)
    if (ctx->linearCapacity < ctx->verts.size())
    {
        if (ctx->linearVerts)
            linearFree(ctx->linearVerts);
        ctx->linearCapacity = ctx->verts.size() + ctx->verts.size() / 4;
        ctx->linearVerts    = static_cast<NVGvertex*>(linearAlloc(ctx->linearCapacity * sizeof(NVGvertex)));
        if (!ctx->linearVerts)
        {
            std::fprintf(stderr, "nanovg citro3d: no linear memory for %zu vertices\n", ctx->verts.size());
            ctx->linearCapacity = 0;
            resetFrame(ctx);
            return;
        }
    }
    std::memcpy(ctx->linearVerts, ctx->verts.data(), ctx->verts.size() * sizeof(NVGvertex));

    C3D_BindProgram(&ctx->program);
    C3D_AttrInfo* attr = C3D_GetAttrInfo();
    AttrInfo_Init(attr);
    AttrInfo_AddLoader(attr, 0, GPU_FLOAT, 4); // v0 = (x, y, u, v)
    C3D_BufInfo* buf = C3D_GetBufInfo();
    BufInfo_Init(buf);
    BufInfo_Add(buf, ctx->linearVerts, sizeof(NVGvertex), 1, 0x0);

    C3D_Mtx proj;
    Mtx_OrthoTilt(&proj, 0.0f, ctx->viewWidth, ctx->viewHeight, 0.0f, -1.0f, 1.0f, false);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, ctx->projLocation, &proj);

    C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
    C3D_AlphaTest(false, GPU_ALWAYS, 0);
    C3D_StencilTest(false, GPU_ALWAYS, 0, 0xFF, 0xFF);
    C3D_SetScissor(GPU_SCISSOR_DISABLE, 0, 0, 0, 0);

    for (Call& call : ctx->calls)
    {
        C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, call.blend[0], call.blend[1], call.blend[2], call.blend[3]);
        if (call.scissor)
            C3D_SetScissor(GPU_SCISSOR_NORMAL, call.scissorBox[0], call.scissorBox[1], call.scissorBox[2],
                call.scissorBox[3]);
        else
            C3D_SetScissor(GPU_SCISSOR_DISABLE, 0, 0, 0, 0);

        switch (call.type)
        {
            case CallType::CONVEX_FILL:
                setPaint(ctx, call.paint);
                drawFans(ctx, call);
                drawStrips(ctx, call); // the fringes, when there are
                break;
            case CallType::FILL:
                // Winding into the stencil, colour off: one face counted up,
                // the other down (nonzero)
                C3D_DepthTest(false, GPU_ALWAYS, (GPU_WRITEMASK)0);
                C3D_StencilTest(true, GPU_ALWAYS, 0, 0xFF, 0xFF);
                C3D_CullFace(GPU_CULL_BACK_CCW);
                C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_INCR_WRAP);
                drawFans(ctx, call);
                C3D_CullFace(GPU_CULL_FRONT_CCW);
                C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_DECR_WRAP);
                drawFans(ctx, call);
                C3D_CullFace(GPU_CULL_NONE);
                // The cover: where the winding is not zero, which it clears
                C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
                setPaint(ctx, call.paint);
                drawStrips(ctx, call);
                C3D_StencilTest(true, GPU_NOTEQUAL, 0, 0xFF, 0xFF);
                C3D_StencilOp(GPU_STENCIL_ZERO, GPU_STENCIL_ZERO, GPU_STENCIL_ZERO);
                C3D_DrawArrays(GPU_TRIANGLE_STRIP, call.triangleOffset, call.triangleCount);
                C3D_StencilTest(false, GPU_ALWAYS, 0, 0xFF, 0xFF);
                break;
            case CallType::STROKE:
                setPaint(ctx, call.paint);
                drawStrips(ctx, call);
                break;
            case CallType::TRIANGLES:
                setPaint(ctx, call.paint);
                C3D_DrawArrays(GPU_TRIANGLES, call.triangleOffset, call.triangleCount);
                break;
        }
    }
    C3D_SetScissor(GPU_SCISSOR_DISABLE, 0, 0, 0, 0);
    // The calls' copies go with them
    C3D_TexBind(0, nullptr);
    resetFrame(ctx);
}

inline void freeTextures(Context* ctx)
{
    for (C3D_Tex& tex : ctx->pendingDeletes)
        C3D_TexDelete(&tex);
    ctx->pendingDeletes.clear();
}

inline int renderCreate(void* uptr)
{
    auto* ctx = static_cast<Context*>(uptr);
    ctx->dvlb = DVLB_ParseFile((u32*)nanovg_citro3d_shbin, nanovg_citro3d_shbin_size);
    if (!ctx->dvlb)
        return 0;
    shaderProgramInit(&ctx->program);
    shaderProgramSetVsh(&ctx->program, &ctx->dvlb->DVLE[0]);
    ctx->projLocation   = shaderInstanceGetUniformLocation(ctx->program.vertexShader, "proj");
    ctx->texRowLocation = shaderInstanceGetUniformLocation(ctx->program.vertexShader, "texRow");
    ctx->texOffLocation = shaderInstanceGetUniformLocation(ctx->program.vertexShader, "texOff");

    // The linear gradients' ramp: 0 to 1 across, clamped
    if (!C3D_TexInit(&ctx->ramp, RAMP_WIDTH, 8, GPU_A8))
        return 0;
    C3D_TexSetFilter(&ctx->ramp, GPU_LINEAR, GPU_LINEAR);
    C3D_TexSetWrap(&ctx->ramp, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    auto* out = static_cast<uint8_t*>(ctx->ramp.data);
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < RAMP_WIDTH; x++)
            out[tiledOffset(x, y, RAMP_WIDTH)] = (uint8_t)(x * 255 / (RAMP_WIDTH - 1));
    C3D_TexFlush(&ctx->ramp);
    return 1;
}

inline void renderDelete(void* uptr)
{
    auto* ctx = static_cast<Context*>(uptr);
    if (!ctx)
        return;
    freeTextures(ctx);
    for (auto& [id, t] : ctx->textures)
        C3D_TexDelete(&t.tex);
    for (auto& g : ctx->gradients)
        C3D_TexDelete(&g.tex);
    if (ctx->ramp.data)
        C3D_TexDelete(&ctx->ramp);
    if (ctx->linearVerts)
        linearFree(ctx->linearVerts);
    if (ctx->dvlb)
    {
        shaderProgramFree(&ctx->program);
        DVLB_Free(ctx->dvlb);
    }
    delete ctx;
}

} // namespace nvgc3d

NVGcontext* nvgCreateC3D(int flags)
{
    (void)flags;
    NVGparams params            = {};
    params.userPtr              = new nvgc3d::Context();
    params.edgeAntiAlias        = 0;
    params.renderCreate         = nvgc3d::renderCreate;
    params.renderCreateTexture  = nvgc3d::renderCreateTexture;
    params.renderDeleteTexture  = nvgc3d::renderDeleteTexture;
    params.renderUpdateTexture  = nvgc3d::renderUpdateTexture;
    params.renderGetTextureSize = nvgc3d::renderGetTextureSize;
    params.renderViewport       = nvgc3d::renderViewport;
    params.renderCancel         = nvgc3d::renderCancel;
    params.renderFlush          = nvgc3d::renderFlush;
    params.renderFill           = nvgc3d::renderFill;
    params.renderStroke         = nvgc3d::renderStroke;
    params.renderTriangles      = nvgc3d::renderTriangles;
    params.renderDelete         = nvgc3d::renderDelete;
    // nvgCreateInternal calls renderDelete when it fails
    return nvgCreateInternal(&params);
}

void nvgDeleteC3D(NVGcontext* ctx)
{
    nvgDeleteInternal(ctx);
}

void nvgC3DFrameBegin(NVGcontext* ctx)
{
    nvgc3d::freeTextures(static_cast<nvgc3d::Context*>(nvgInternalParams(ctx)->userPtr));
}

#endif /* NANOVG_CITRO3D_IMPLEMENTATION */

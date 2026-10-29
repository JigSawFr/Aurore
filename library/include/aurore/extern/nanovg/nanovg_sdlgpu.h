//
// nanovg's renderer on SDL3's GPU API (SDL_GPU): Vulkan, Metal and
// Direct3D 12. A port of nanovg_gl.h's GL3 renderer, with the same shaders
// (nvg_shader/sdlgpu, compiled by tools/make_sdlgpu_shaders.py) and the same
// draw calls, stencil passes included.
//
// What differs from GL:
// - SDL_GPU's pipelines are immutable: one per stencil state, primitive and
//   blend, made when first used.
// - It has no triangle fans: a fill's fan is written as a triangle list.
// - Drawing happens in a render pass of the command buffer the caller gives
//   (nvgSDLGPUSetTarget), into its colour and depth-stencil textures; the
//   caller submits it.
// - A texture is uploaded with a command buffer of its own, submitted at
//   once, so it is there before the frame's draws, as in GL.
//
// Copyright 2026 JigSawFr, from nanovg_gl.h:
// Copyright (c) 2009-2013 Mikko Mononen memon@inside.org
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
#ifndef NANOVG_SDLGPU_H
#define NANOVG_SDLGPU_H

#include <SDL3/SDL_gpu.h>

#include "nanovg.h"

#ifdef __cplusplus
extern "C"
{
#endif

// As the GL renderer has them
#ifndef NVG_ANTIALIAS
// Geometry based anti-aliasing
#define NVG_ANTIALIAS 1
// Strokes through the stencil buffer: overlaps drawn once
#define NVG_STENCIL_STROKES 2
#endif
// On top of NVGimageFlags: a texture the renderer does not release
#define NVG_SDLGPU_IMAGE_NODELETE (1 << 16)

// The shader formats the renderer brings: what SDL_CreateGPUDevice is asked
// for
#define NVG_SDLGPU_SHADER_FORMATS (SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_DXIL)

    // colorFormat and depthStencilFormat: the targets' (nvgSDLGPUSetTarget).
    // flags: NVG_ANTIALIAS, NVG_STENCIL_STROKES.
    NVGcontext* nvgCreateSDLGPU(SDL_GPUDevice* device, SDL_GPUTextureFormat colorFormat,
        SDL_GPUTextureFormat depthStencilFormat, int flags);
    void nvgDeleteSDLGPU(NVGcontext* ctx);

    // What the next nvgEndFrame draws into: the frame's command buffer, a colour
    // texture and a depth-stencil one, cleared first when clear is not NULL (and
    // even with nothing drawn)
    void nvgSDLGPUSetTarget(NVGcontext* ctx, SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* color,
        SDL_GPUTexture* depthStencil, const SDL_FColor* clear);

#ifdef __cplusplus
}
#endif

#endif // NANOVG_SDLGPU_H

#ifdef NANOVG_SDLGPU_IMPLEMENTATION

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>

#include "nvg_shader/sdlgpu/sdlgpu_shaders.h"

enum SDLGPUNVGshaderType
{
    SDLGPUNVG_SHADER_FILLGRAD,
    SDLGPUNVG_SHADER_FILLIMG,
    SDLGPUNVG_SHADER_SIMPLE,
    SDLGPUNVG_SHADER_IMG
};

enum SDLGPUNVGcallType
{
    SDLGPUNVG_NONE = 0,
    SDLGPUNVG_FILL,
    SDLGPUNVG_CONVEXFILL,
    SDLGPUNVG_STROKE,
    SDLGPUNVG_TRIANGLES,
    SDLGPUNVG_CONVEXFILL_STENCIL,
    SDLGPUNVG_CONVEXFILL_STENCIL_CLEAR,
};

// The stencil states of nanovg_gl.h's passes
enum SDLGPUNVGstencil
{
    SDLGPUNVG_STENCIL_NONE, // no test
    SDLGPUNVG_STENCIL_WINDING, // always; front faces add, back faces take away
    SDLGPUNVG_STENCIL_EQUAL_KEEP, // where 0
    SDLGPUNVG_STENCIL_NOTEQUAL_ZERO, // where not 0, back to 0
    SDLGPUNVG_STENCIL_EQUAL_INCR, // where 0, then 1: no overlap
    SDLGPUNVG_STENCIL_ALWAYS_ZERO, // back to 0
    SDLGPUNVG_STENCIL_ALWAYS_REPLACE, // the reference (1), for nvgStencil
    SDLGPUNVG_STENCIL_COUNT
};

typedef struct SDLGPUNVGtexture
{
    int id;
    SDL_GPUTexture* tex;
    int width, height;
    int type;
    int flags;
    int levels;
} SDLGPUNVGtexture;

typedef struct SDLGPUNVGblend
{
    SDL_GPUBlendFactor srcRGB;
    SDL_GPUBlendFactor dstRGB;
    SDL_GPUBlendFactor srcAlpha;
    SDL_GPUBlendFactor dstAlpha;
} SDLGPUNVGblend;

typedef struct SDLGPUNVGcall
{
    int type;
    int image;
    int pathOffset;
    int pathCount;
    int triangleOffset;
    int triangleCount;
    int uniformOffset;
    SDLGPUNVGblend blend;
} SDLGPUNVGcall;

typedef struct SDLGPUNVGpath
{
    int fillOffset; // a triangle list (the GL renderer's fan, unfolded)
    int fillCount;
    int strokeOffset; // a triangle strip
    int strokeCount;
} SDLGPUNVGpath;

// The fragment shader's uniforms: nanovg_gl.h's array of 11 vec4
typedef struct SDLGPUNVGfragUniforms
{
    float scissorMat[12]; // 3 vec4
    float paintMat[12];
    NVGcolor innerCol;
    NVGcolor outerCol;
    float scissorExt[2];
    float scissorScale[2];
    float extent[2];
    float radius;
    float feather;
    float strokeMult;
    float strokeThr;
    float texType;
    float type;
} SDLGPUNVGfragUniforms;

typedef struct SDLGPUNVGpipeline
{
    Uint32 key;
    SDL_GPUGraphicsPipeline* pipeline;
} SDLGPUNVGpipeline;

typedef struct SDLGPUNVGcontext
{
    SDL_GPUDevice* device;
    SDL_GPUTextureFormat colorFormat;
    SDL_GPUTextureFormat depthStencilFormat;
    int flags;

    SDL_GPUShader* vert;
    SDL_GPUShader* frag;
    SDLGPUNVGpipeline* pipelines;
    int npipelines;
    int cpipelines;
    SDL_GPUSampler* samplers[16];

    SDLGPUNVGtexture* textures;
    int ntextures;
    int ctextures;
    int textureId;
    int dummyTex;

    SDL_GPUBuffer* vertBuf;
    SDL_GPUTransferBuffer* vertTransfer;
    Uint32 vertBufSize;

    float view[2];

    // The frame's target (nvgSDLGPUSetTarget)
    SDL_GPUCommandBuffer* cmd;
    SDL_GPUTexture* color;
    SDL_GPUTexture* depthStencil;
    int clear;
    SDL_FColor clearColor;

    // Per frame buffers
    SDLGPUNVGcall* calls;
    int ccalls;
    int ncalls;
    SDLGPUNVGpath* paths;
    int cpaths;
    int npaths;
    NVGvertex* verts;
    int cverts;
    int nverts;
    SDLGPUNVGfragUniforms* uniforms;
    int cuniforms;
    int nuniforms;
} SDLGPUNVGcontext;

static int sdlgpunvg__maxi(int a, int b) { return a > b ? a : b; }

static SDLGPUNVGtexture* sdlgpunvg__allocTexture(SDLGPUNVGcontext* gpu)
{
    SDLGPUNVGtexture* tex = NULL;
    int i;
    for (i = 0; i < gpu->ntextures; i++)
    {
        if (gpu->textures[i].id == 0)
        {
            tex = &gpu->textures[i];
            break;
        }
    }
    if (tex == NULL)
    {
        if (gpu->ntextures + 1 > gpu->ctextures)
        {
            int ctextures              = sdlgpunvg__maxi(gpu->ntextures + 1, 4) + gpu->ctextures / 2;
            SDLGPUNVGtexture* textures = (SDLGPUNVGtexture*)realloc(gpu->textures, sizeof(SDLGPUNVGtexture) * ctextures);
            if (textures == NULL)
                return NULL;
            gpu->textures  = textures;
            gpu->ctextures = ctextures;
        }
        tex = &gpu->textures[gpu->ntextures++];
    }
    memset(tex, 0, sizeof(*tex));
    tex->id = ++gpu->textureId;
    return tex;
}

static SDLGPUNVGtexture* sdlgpunvg__findTexture(SDLGPUNVGcontext* gpu, int id)
{
    int i;
    for (i = 0; i < gpu->ntextures; i++)
        if (gpu->textures[i].id == id)
            return &gpu->textures[i];
    return NULL;
}

static int sdlgpunvg__deleteTexture(SDLGPUNVGcontext* gpu, int id)
{
    int i;
    for (i = 0; i < gpu->ntextures; i++)
    {
        if (gpu->textures[i].id == id)
        {
            // Released once the GPU is done with it
            if (gpu->textures[i].tex != NULL && (gpu->textures[i].flags & NVG_SDLGPU_IMAGE_NODELETE) == 0)
                SDL_ReleaseGPUTexture(gpu->device, gpu->textures[i].tex);
            memset(&gpu->textures[i], 0, sizeof(gpu->textures[i]));
            return 1;
        }
    }
    return 0;
}

static SDL_GPUShader* sdlgpunvg__createShader(SDLGPUNVGcontext* gpu, SDL_GPUShaderStage stage,
    const unsigned char* spirv, size_t spirvSize, const char* msl,
    const unsigned char* dxil, size_t dxilSize, Uint32 samplers,
    Uint32 uniformBuffers)
{
    SDL_GPUShaderCreateInfo info;
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(gpu->device);
    memset(&info, 0, sizeof(info));
    info.stage               = stage;
    info.num_samplers        = samplers;
    info.num_uniform_buffers = uniformBuffers;
    if (formats & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        info.format     = SDL_GPU_SHADERFORMAT_SPIRV;
        info.code       = spirv;
        info.code_size  = spirvSize;
        info.entrypoint = "main";
    }
    else if (formats & SDL_GPU_SHADERFORMAT_DXIL)
    {
        info.format     = SDL_GPU_SHADERFORMAT_DXIL;
        info.code       = dxil;
        info.code_size  = dxilSize;
        info.entrypoint = "main";
    }
    else if (formats & SDL_GPU_SHADERFORMAT_MSL)
    {
        info.format     = SDL_GPU_SHADERFORMAT_MSL;
        info.code       = (const Uint8*)msl;
        info.code_size  = strlen(msl);
        info.entrypoint = "main0"; // spirv-cross's name for main
    }
    else
    {
        SDL_SetError("nanovg: the GPU takes none of SPIR-V, DXIL and MSL");
        return NULL;
    }
    return SDL_CreateGPUShader(gpu->device, &info);
}

static SDL_GPUBlendFactor sdlgpunvg__blendFactor(int factor)
{
    switch (factor)
    {
        case NVG_ZERO:
            return SDL_GPU_BLENDFACTOR_ZERO;
        case NVG_ONE:
            return SDL_GPU_BLENDFACTOR_ONE;
        case NVG_SRC_COLOR:
            return SDL_GPU_BLENDFACTOR_SRC_COLOR;
        case NVG_ONE_MINUS_SRC_COLOR:
            return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR;
        case NVG_DST_COLOR:
            return SDL_GPU_BLENDFACTOR_DST_COLOR;
        case NVG_ONE_MINUS_DST_COLOR:
            return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR;
        case NVG_SRC_ALPHA:
            return SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        case NVG_ONE_MINUS_SRC_ALPHA:
            return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        case NVG_DST_ALPHA:
            return SDL_GPU_BLENDFACTOR_DST_ALPHA;
        case NVG_ONE_MINUS_DST_ALPHA:
            return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
        case NVG_SRC_ALPHA_SATURATE:
            return SDL_GPU_BLENDFACTOR_SRC_ALPHA_SATURATE;
        default:
            return SDL_GPU_BLENDFACTOR_INVALID;
    }
}

static SDLGPUNVGblend sdlgpunvg__blendCompositeOperation(NVGcompositeOperationState op)
{
    SDLGPUNVGblend blend;
    blend.srcRGB   = sdlgpunvg__blendFactor(op.srcRGB);
    blend.dstRGB   = sdlgpunvg__blendFactor(op.dstRGB);
    blend.srcAlpha = sdlgpunvg__blendFactor(op.srcAlpha);
    blend.dstAlpha = sdlgpunvg__blendFactor(op.dstAlpha);
    if (blend.srcRGB == SDL_GPU_BLENDFACTOR_INVALID || blend.dstRGB == SDL_GPU_BLENDFACTOR_INVALID
        || blend.srcAlpha == SDL_GPU_BLENDFACTOR_INVALID || blend.dstAlpha == SDL_GPU_BLENDFACTOR_INVALID)
    {
        blend.srcRGB   = SDL_GPU_BLENDFACTOR_ONE;
        blend.dstRGB   = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        blend.srcAlpha = SDL_GPU_BLENDFACTOR_ONE;
        blend.dstAlpha = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    }
    return blend;
}

static SDL_GPUStencilOpState sdlgpunvg__stencilOp(SDL_GPUCompareOp compare, SDL_GPUStencilOp pass)
{
    SDL_GPUStencilOpState op;
    op.compare_op    = compare;
    op.fail_op       = SDL_GPU_STENCILOP_KEEP;
    op.depth_fail_op = SDL_GPU_STENCILOP_KEEP;
    op.pass_op       = pass;
    return op;
}

// A pipeline: its stencil state, primitive (strip or list), whether it
// writes colour and culls back faces, its blend
static Uint32 sdlgpunvg__pipelineKey(int stencil, int strip, int colorWrite, int cull, const SDLGPUNVGblend* blend)
{
    return (Uint32)stencil | (Uint32)strip << 3 | (Uint32)colorWrite << 4 | (Uint32)cull << 5
        | (Uint32)blend->srcRGB << 8 | (Uint32)blend->dstRGB << 13 | (Uint32)blend->srcAlpha << 18
        | (Uint32)blend->dstAlpha << 23;
}

static SDL_GPUGraphicsPipeline* sdlgpunvg__pipeline(SDLGPUNVGcontext* gpu, int stencil, int strip, int colorWrite,
    int cull, const SDLGPUNVGblend* blend)
{
    Uint32 key = sdlgpunvg__pipelineKey(stencil, strip, colorWrite, cull, blend);
    SDL_GPUGraphicsPipelineCreateInfo info;
    SDL_GPUColorTargetDescription target;
    SDL_GPUVertexBufferDescription buffer;
    SDL_GPUVertexAttribute attributes[2];
    SDL_GPUGraphicsPipeline* pipeline;
    int i;

    for (i = 0; i < gpu->npipelines; i++)
        if (gpu->pipelines[i].key == key)
            return gpu->pipelines[i].pipeline;

    memset(&info, 0, sizeof(info));
    memset(&target, 0, sizeof(target));
    memset(&buffer, 0, sizeof(buffer));
    memset(attributes, 0, sizeof(attributes));

    buffer.slot            = 0;
    buffer.pitch           = sizeof(NVGvertex);
    buffer.input_rate      = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    attributes[0].location = 0;
    attributes[0].format   = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attributes[0].offset   = 0;
    attributes[1].location = 1;
    attributes[1].format   = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attributes[1].offset   = 2 * sizeof(float);

    target.format                              = gpu->colorFormat;
    target.blend_state.enable_blend            = true;
    target.blend_state.src_color_blendfactor   = blend->srcRGB;
    target.blend_state.dst_color_blendfactor   = blend->dstRGB;
    target.blend_state.color_blend_op          = SDL_GPU_BLENDOP_ADD;
    target.blend_state.src_alpha_blendfactor   = blend->srcAlpha;
    target.blend_state.dst_alpha_blendfactor   = blend->dstAlpha;
    target.blend_state.alpha_blend_op          = SDL_GPU_BLENDOP_ADD;
    target.blend_state.enable_color_write_mask = true;
    target.blend_state.color_write_mask        = colorWrite ? (SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A) : 0;

    info.vertex_shader                                 = gpu->vert;
    info.fragment_shader                               = gpu->frag;
    info.vertex_input_state.vertex_buffer_descriptions = &buffer;
    info.vertex_input_state.num_vertex_buffers         = 1;
    info.vertex_input_state.vertex_attributes          = attributes;
    info.vertex_input_state.num_vertex_attributes      = 2;
    info.primitive_type                                = strip ? SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP : SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.fill_mode                    = SDL_GPU_FILLMODE_FILL;
    info.rasterizer_state.cull_mode                    = cull ? SDL_GPU_CULLMODE_BACK : SDL_GPU_CULLMODE_NONE;
    info.rasterizer_state.front_face                   = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    info.target_info.color_target_descriptions         = &target;
    info.target_info.num_color_targets                 = 1;
    info.target_info.depth_stencil_format              = gpu->depthStencilFormat;
    info.target_info.has_depth_stencil_target          = true;

    info.depth_stencil_state.compare_mask        = 0xff;
    info.depth_stencil_state.write_mask          = 0xff;
    info.depth_stencil_state.enable_stencil_test = stencil != SDLGPUNVG_STENCIL_NONE;
    switch (stencil)
    {
        case SDLGPUNVG_STENCIL_WINDING:
            info.depth_stencil_state.front_stencil_state = sdlgpunvg__stencilOp(SDL_GPU_COMPAREOP_ALWAYS, SDL_GPU_STENCILOP_INCREMENT_AND_WRAP);
            info.depth_stencil_state.back_stencil_state  = sdlgpunvg__stencilOp(SDL_GPU_COMPAREOP_ALWAYS, SDL_GPU_STENCILOP_DECREMENT_AND_WRAP);
            break;
        case SDLGPUNVG_STENCIL_EQUAL_KEEP:
            info.depth_stencil_state.front_stencil_state = sdlgpunvg__stencilOp(SDL_GPU_COMPAREOP_EQUAL, SDL_GPU_STENCILOP_KEEP);
            break;
        case SDLGPUNVG_STENCIL_NOTEQUAL_ZERO:
            info.depth_stencil_state.front_stencil_state         = sdlgpunvg__stencilOp(SDL_GPU_COMPAREOP_NOT_EQUAL, SDL_GPU_STENCILOP_ZERO);
            info.depth_stencil_state.front_stencil_state.fail_op = SDL_GPU_STENCILOP_ZERO;
            break;
        case SDLGPUNVG_STENCIL_EQUAL_INCR:
            info.depth_stencil_state.front_stencil_state = sdlgpunvg__stencilOp(SDL_GPU_COMPAREOP_EQUAL, SDL_GPU_STENCILOP_INCREMENT_AND_CLAMP);
            break;
        case SDLGPUNVG_STENCIL_ALWAYS_ZERO:
            info.depth_stencil_state.front_stencil_state = sdlgpunvg__stencilOp(SDL_GPU_COMPAREOP_ALWAYS, SDL_GPU_STENCILOP_ZERO);
            break;
        case SDLGPUNVG_STENCIL_ALWAYS_REPLACE:
            info.depth_stencil_state.front_stencil_state = sdlgpunvg__stencilOp(SDL_GPU_COMPAREOP_ALWAYS, SDL_GPU_STENCILOP_REPLACE);
            break;
        default:
            break;
    }
    if (stencil != SDLGPUNVG_STENCIL_WINDING)
        info.depth_stencil_state.back_stencil_state = info.depth_stencil_state.front_stencil_state;

    pipeline = SDL_CreateGPUGraphicsPipeline(gpu->device, &info);
    if (pipeline == NULL)
    {
        SDL_Log("nanovg: no SDL_GPU pipeline: %s", SDL_GetError());
        return NULL;
    }
    if (gpu->npipelines + 1 > gpu->cpipelines)
    {
        int cpipelines               = sdlgpunvg__maxi(gpu->npipelines + 1, 16) * 2;
        SDLGPUNVGpipeline* pipelines = (SDLGPUNVGpipeline*)realloc(gpu->pipelines, sizeof(SDLGPUNVGpipeline) * cpipelines);
        if (pipelines == NULL)
        {
            SDL_ReleaseGPUGraphicsPipeline(gpu->device, pipeline);
            return NULL;
        }
        gpu->pipelines  = pipelines;
        gpu->cpipelines = cpipelines;
    }
    gpu->pipelines[gpu->npipelines].key      = key;
    gpu->pipelines[gpu->npipelines].pipeline = pipeline;
    gpu->npipelines++;
    return pipeline;
}

// The sampler of an image's flags: nearest or linear, repeated or clamped,
// with or without its mipmaps
static SDL_GPUSampler* sdlgpunvg__sampler(SDLGPUNVGcontext* gpu, int imageFlags)
{
    int index = ((imageFlags & NVG_IMAGE_NEAREST) ? 1 : 0) | ((imageFlags & NVG_IMAGE_REPEATX) ? 2 : 0)
        | ((imageFlags & NVG_IMAGE_REPEATY) ? 4 : 0) | ((imageFlags & NVG_IMAGE_GENERATE_MIPMAPS) ? 8 : 0);
    if (gpu->samplers[index] == NULL)
    {
        SDL_GPUSamplerCreateInfo info;
        memset(&info, 0, sizeof(info));
        info.min_filter = info.mag_filter = (index & 1) ? SDL_GPU_FILTER_NEAREST : SDL_GPU_FILTER_LINEAR;
        info.mipmap_mode                  = (index & 1) ? SDL_GPU_SAMPLERMIPMAPMODE_NEAREST : SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        info.address_mode_u               = (index & 2) ? SDL_GPU_SAMPLERADDRESSMODE_REPEAT : SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        info.address_mode_v               = (index & 4) ? SDL_GPU_SAMPLERADDRESSMODE_REPEAT : SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        info.address_mode_w               = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        info.min_lod                      = 0.0f;
        info.max_lod                      = (index & 8) ? 1000.0f : 0.0f;
        gpu->samplers[index]              = SDL_CreateGPUSampler(gpu->device, &info);
    }
    return gpu->samplers[index];
}

static int sdlgpunvg__renderCreateTexture(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data);
static int sdlgpunvg__renderUpdateTexture(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data);

static int sdlgpunvg__renderCreate(void* uptr)
{
    SDLGPUNVGcontext* gpu = (SDLGPUNVGcontext*)uptr;
    int aa                = (gpu->flags & NVG_ANTIALIAS) != 0;

// A shader of sdlgpu_shaders.h, in each of its formats
#define SDLGPUNVG_SHADER(name)                                                                                       \
    sdlgpu_nvg_##name##_spirv, sizeof(sdlgpu_nvg_##name##_spirv), sdlgpu_nvg_##name##_msl, sdlgpu_nvg_##name##_dxil, \
        sizeof(sdlgpu_nvg_##name##_dxil)
    gpu->vert = sdlgpunvg__createShader(gpu, SDL_GPU_SHADERSTAGE_VERTEX, SDLGPUNVG_SHADER(vert), 0, 1);
    gpu->frag = aa ? sdlgpunvg__createShader(gpu, SDL_GPU_SHADERSTAGE_FRAGMENT, SDLGPUNVG_SHADER(frag_aa), 1, 1)
                   : sdlgpunvg__createShader(gpu, SDL_GPU_SHADERSTAGE_FRAGMENT, SDLGPUNVG_SHADER(frag), 1, 1);
#undef SDLGPUNVG_SHADER
    if (gpu->vert == NULL || gpu->frag == NULL)
    {
        SDL_Log("nanovg: no SDL_GPU shader: %s", SDL_GetError());
        return 0;
    }

    // Bound when no image is: a texture is always sampled
    gpu->dummyTex = sdlgpunvg__renderCreateTexture(gpu, NVG_TEXTURE_ALPHA, 1, 1, 0, NULL);
    return gpu->dummyTex != 0;
}

static int sdlgpunvg__mipLevels(int w, int h)
{
    int levels = 1, size = sdlgpunvg__maxi(w, h);
    while (size > 1)
    {
        size /= 2;
        levels++;
    }
    return levels;
}

static int sdlgpunvg__renderCreateTexture(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data)
{
    SDLGPUNVGcontext* gpu = (SDLGPUNVGcontext*)uptr;
    SDLGPUNVGtexture* tex = sdlgpunvg__allocTexture(gpu);
    SDL_GPUTextureCreateInfo info;

    if (tex == NULL)
        return 0;

    memset(&info, 0, sizeof(info));
    info.type                 = SDL_GPU_TEXTURETYPE_2D;
    info.format               = type == NVG_TEXTURE_RGBA ? SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM : SDL_GPU_TEXTUREFORMAT_R8_UNORM;
    info.usage                = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    info.width                = (Uint32)w;
    info.height               = (Uint32)h;
    info.layer_count_or_depth = 1;
    info.num_levels           = 1;
    if (imageFlags & NVG_IMAGE_GENERATE_MIPMAPS)
    {
        // SDL_GenerateMipmapsForGPUTexture draws them
        info.usage |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        info.num_levels = (Uint32)sdlgpunvg__mipLevels(w, h);
    }

    tex->tex = SDL_CreateGPUTexture(gpu->device, &info);
    if (tex->tex == NULL)
    {
        SDL_Log("nanovg: no SDL_GPU texture %dx%d: %s", w, h, SDL_GetError());
        memset(tex, 0, sizeof(*tex));
        return 0;
    }
    tex->width  = w;
    tex->height = h;
    tex->type   = type;
    tex->flags  = imageFlags;
    tex->levels = (int)info.num_levels;

    if (data != NULL)
        sdlgpunvg__renderUpdateTexture(gpu, tex->id, 0, 0, w, h, data);
    else
    {
        // GL leaves it undefined; zeros here, as a font atlas expects
        size_t size          = (size_t)w * h * (type == NVG_TEXTURE_RGBA ? 4 : 1);
        unsigned char* zeros = (unsigned char*)calloc(1, size);
        if (zeros != NULL)
        {
            sdlgpunvg__renderUpdateTexture(gpu, tex->id, 0, 0, w, h, zeros);
            free(zeros);
        }
    }
    return tex->id;
}

static int sdlgpunvg__renderDeleteTexture(void* uptr, int image)
{
    return sdlgpunvg__deleteTexture((SDLGPUNVGcontext*)uptr, image);
}

// data is the whole image, as in GL (its rows are the texture's width)
static int sdlgpunvg__renderUpdateTexture(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data)
{
    SDLGPUNVGcontext* gpu = (SDLGPUNVGcontext*)uptr;
    SDLGPUNVGtexture* tex = sdlgpunvg__findTexture(gpu, image);
    SDL_GPUTransferBufferCreateInfo transferInfo;
    SDL_GPUTransferBuffer* transfer;
    SDL_GPUCommandBuffer* cmd;
    SDL_GPUCopyPass* copy;
    SDL_GPUTextureTransferInfo source;
    SDL_GPUTextureRegion region;
    int bpp;
    size_t rowBytes, size;
    unsigned char* mapped;

    if (tex == NULL || w <= 0 || h <= 0)
        return 0;
    bpp      = tex->type == NVG_TEXTURE_RGBA ? 4 : 1;
    rowBytes = (size_t)tex->width * bpp;
    size     = rowBytes * h;

    memset(&transferInfo, 0, sizeof(transferInfo));
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size  = (Uint32)size;
    transfer           = SDL_CreateGPUTransferBuffer(gpu->device, &transferInfo);
    if (transfer == NULL)
        return 0;
    mapped = (unsigned char*)SDL_MapGPUTransferBuffer(gpu->device, transfer, false);
    if (mapped == NULL)
    {
        SDL_ReleaseGPUTransferBuffer(gpu->device, transfer);
        return 0;
    }
    // The rows y to y + h, whole: the region is picked in them by offset
    memcpy(mapped, data + rowBytes * y, size);
    SDL_UnmapGPUTransferBuffer(gpu->device, transfer);

    memset(&source, 0, sizeof(source));
    source.transfer_buffer = transfer;
    source.offset          = (Uint32)(x * bpp);
    source.pixels_per_row  = (Uint32)tex->width;
    source.rows_per_layer  = (Uint32)h;
    memset(&region, 0, sizeof(region));
    region.texture = tex->tex;
    region.x       = (Uint32)x;
    region.y       = (Uint32)y;
    region.w       = (Uint32)w;
    region.h       = (Uint32)h;
    region.d       = 1;

    cmd = SDL_AcquireGPUCommandBuffer(gpu->device);
    if (cmd == NULL)
    {
        SDL_ReleaseGPUTransferBuffer(gpu->device, transfer);
        return 0;
    }
    copy = SDL_BeginGPUCopyPass(cmd);
    SDL_UploadToGPUTexture(copy, &source, &region, false);
    SDL_EndGPUCopyPass(copy);
    if (tex->levels > 1)
        SDL_GenerateMipmapsForGPUTexture(cmd, tex->tex);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(gpu->device, transfer);
    return 1;
}

static int sdlgpunvg__renderGetTextureSize(void* uptr, int image, int* w, int* h)
{
    SDLGPUNVGtexture* tex = sdlgpunvg__findTexture((SDLGPUNVGcontext*)uptr, image);
    if (tex == NULL)
        return 0;
    *w = tex->width;
    *h = tex->height;
    return 1;
}

static void sdlgpunvg__xformToMat3x4(float* m3, float* t)
{
    m3[0]  = t[0];
    m3[1]  = t[1];
    m3[2]  = 0.0f;
    m3[3]  = 0.0f;
    m3[4]  = t[2];
    m3[5]  = t[3];
    m3[6]  = 0.0f;
    m3[7]  = 0.0f;
    m3[8]  = t[4];
    m3[9]  = t[5];
    m3[10] = 1.0f;
    m3[11] = 0.0f;
}

static NVGcolor sdlgpunvg__premulColor(NVGcolor c)
{
    c.r *= c.a;
    c.g *= c.a;
    c.b *= c.a;
    return c;
}

static int sdlgpunvg__convertPaint(SDLGPUNVGcontext* gpu, SDLGPUNVGfragUniforms* frag, NVGpaint* paint,
    NVGscissor* scissor, float width, float fringe, float strokeThr)
{
    SDLGPUNVGtexture* tex = NULL;
    float invxform[6];

    memset(frag, 0, sizeof(*frag));
    frag->innerCol = sdlgpunvg__premulColor(paint->innerColor);
    frag->outerCol = sdlgpunvg__premulColor(paint->outerColor);

    if (scissor->extent[0] < -0.5f || scissor->extent[1] < -0.5f)
    {
        memset(frag->scissorMat, 0, sizeof(frag->scissorMat));
        frag->scissorExt[0]   = 1.0f;
        frag->scissorExt[1]   = 1.0f;
        frag->scissorScale[0] = 1.0f;
        frag->scissorScale[1] = 1.0f;
    }
    else
    {
        nvgTransformInverse(invxform, scissor->xform);
        sdlgpunvg__xformToMat3x4(frag->scissorMat, invxform);
        frag->scissorExt[0]   = scissor->extent[0];
        frag->scissorExt[1]   = scissor->extent[1];
        frag->scissorScale[0] = sqrtf(scissor->xform[0] * scissor->xform[0] + scissor->xform[2] * scissor->xform[2]) / fringe;
        frag->scissorScale[1] = sqrtf(scissor->xform[1] * scissor->xform[1] + scissor->xform[3] * scissor->xform[3]) / fringe;
    }

    memcpy(frag->extent, paint->extent, sizeof(frag->extent));
    frag->strokeMult = (width * 0.5f + fringe * 0.5f) / fringe;
    frag->strokeThr  = strokeThr;

    if (paint->image != 0)
    {
        tex = sdlgpunvg__findTexture(gpu, paint->image);
        if (tex == NULL)
            return 0;
        if ((tex->flags & NVG_IMAGE_FLIPY) != 0)
        {
            float m1[6], m2[6];
            nvgTransformTranslate(m1, 0.0f, frag->extent[1] * 0.5f);
            nvgTransformMultiply(m1, paint->xform);
            nvgTransformScale(m2, 1.0f, -1.0f);
            nvgTransformMultiply(m2, m1);
            nvgTransformTranslate(m1, 0.0f, -frag->extent[1] * 0.5f);
            nvgTransformMultiply(m1, m2);
            nvgTransformInverse(invxform, m1);
        }
        else
            nvgTransformInverse(invxform, paint->xform);
        frag->type = SDLGPUNVG_SHADER_FILLIMG;
        if (tex->type == NVG_TEXTURE_RGBA)
            frag->texType = scissor->stencilFlag ? 3.0f : ((tex->flags & NVG_IMAGE_PREMULTIPLIED) ? 0.0f : 1.0f);
        else
            frag->texType = 2.0f;
    }
    else
    {
        frag->type    = SDLGPUNVG_SHADER_FILLGRAD;
        frag->radius  = paint->radius;
        frag->feather = paint->feather;
        nvgTransformInverse(invxform, paint->xform);
    }
    sdlgpunvg__xformToMat3x4(frag->paintMat, invxform);
    return 1;
}

static void sdlgpunvg__renderViewport(void* uptr, float width, float height, float devicePixelRatio)
{
    SDLGPUNVGcontext* gpu = (SDLGPUNVGcontext*)uptr;
    NVG_NOTUSED(devicePixelRatio);
    gpu->view[0] = width;
    gpu->view[1] = height;
}

static void sdlgpunvg__renderCancel(void* uptr)
{
    SDLGPUNVGcontext* gpu = (SDLGPUNVGcontext*)uptr;
    gpu->nverts           = 0;
    gpu->npaths           = 0;
    gpu->ncalls           = 0;
    gpu->nuniforms        = 0;
}

// What a draw needs besides its pipeline: its uniforms and its image
static void sdlgpunvg__setUniforms(SDLGPUNVGcontext* gpu, SDL_GPURenderPass* pass, int uniformOffset, int image)
{
    SDLGPUNVGtexture* tex = image != 0 ? sdlgpunvg__findTexture(gpu, image) : NULL;
    SDL_GPUTextureSamplerBinding binding;
    if (tex == NULL)
        tex = sdlgpunvg__findTexture(gpu, gpu->dummyTex);
    SDL_PushGPUFragmentUniformData(gpu->cmd, 0, &gpu->uniforms[uniformOffset], sizeof(SDLGPUNVGfragUniforms));
    binding.texture = tex->tex;
    binding.sampler = sdlgpunvg__sampler(gpu, tex->levels > 1 ? tex->flags : tex->flags & ~NVG_IMAGE_GENERATE_MIPMAPS);
    SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
}

static int sdlgpunvg__bind(SDLGPUNVGcontext* gpu, SDL_GPURenderPass* pass, int stencil, int strip, int colorWrite,
    int cull, const SDLGPUNVGblend* blend)
{
    SDL_GPUGraphicsPipeline* pipeline = sdlgpunvg__pipeline(gpu, stencil, strip, colorWrite, cull, blend);
    if (pipeline == NULL)
        return 0;
    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    return 1;
}

static void sdlgpunvg__draw(SDL_GPURenderPass* pass, int offset, int count)
{
    if (count > 0)
        SDL_DrawGPUPrimitives(pass, (Uint32)count, 1, (Uint32)offset, 0);
}

// nvgStencil's mask: while it is set, what is drawn stays out of it (the GL
// renderer's stencil test, left on after GLNVG_CONVEXFILL_STENCIL)
typedef struct SDLGPUNVGpassState
{
    SDL_GPURenderPass* pass;
    int clipped;
} SDLGPUNVGpassState;

static void sdlgpunvg__fill(SDLGPUNVGcontext* gpu, SDLGPUNVGpassState* state, SDLGPUNVGcall* call)
{
    SDLGPUNVGpath* paths    = &gpu->paths[call->pathOffset];
    SDL_GPURenderPass* pass = state->pass;
    int i, npaths = call->pathCount;

    // The shapes, into the stencil only
    if (sdlgpunvg__bind(gpu, pass, SDLGPUNVG_STENCIL_WINDING, 0, 0, 0, &call->blend))
    {
        sdlgpunvg__setUniforms(gpu, pass, call->uniformOffset, 0);
        for (i = 0; i < npaths; i++)
            sdlgpunvg__draw(pass, paths[i].fillOffset, paths[i].fillCount);
    }

    sdlgpunvg__setUniforms(gpu, pass, call->uniformOffset + 1, call->image);
    // The anti-aliased fringes, outside the shape
    if ((gpu->flags & NVG_ANTIALIAS) && sdlgpunvg__bind(gpu, pass, SDLGPUNVG_STENCIL_EQUAL_KEEP, 1, 1, 1, &call->blend))
        for (i = 0; i < npaths; i++)
            sdlgpunvg__draw(pass, paths[i].strokeOffset, paths[i].strokeCount);

    // The fill, where the stencil is set, which it clears
    if (sdlgpunvg__bind(gpu, pass, SDLGPUNVG_STENCIL_NOTEQUAL_ZERO, 1, 1, 1, &call->blend))
        sdlgpunvg__draw(pass, call->triangleOffset, call->triangleCount);

    // GL turns its stencil test off here, nvgStencil's mask with it
    state->clipped = 0;
}

static void sdlgpunvg__convexFill(SDLGPUNVGcontext* gpu, SDLGPUNVGpassState* state, SDLGPUNVGcall* call, int stencil,
    int colorWrite)
{
    SDLGPUNVGpath* paths    = &gpu->paths[call->pathOffset];
    SDL_GPURenderPass* pass = state->pass;
    int i, npaths = call->pathCount;

    sdlgpunvg__setUniforms(gpu, pass, call->uniformOffset, call->image);
    if (sdlgpunvg__bind(gpu, pass, stencil, 0, colorWrite, 1, &call->blend))
        for (i = 0; i < npaths; i++)
            sdlgpunvg__draw(pass, paths[i].fillOffset, paths[i].fillCount);
    // The fringes
    if (sdlgpunvg__bind(gpu, pass, stencil, 1, colorWrite, 1, &call->blend))
        for (i = 0; i < npaths; i++)
            sdlgpunvg__draw(pass, paths[i].strokeOffset, paths[i].strokeCount);
}

static void sdlgpunvg__stroke(SDLGPUNVGcontext* gpu, SDLGPUNVGpassState* state, SDLGPUNVGcall* call)
{
    SDLGPUNVGpath* paths    = &gpu->paths[call->pathOffset];
    SDL_GPURenderPass* pass = state->pass;
    int npaths              = call->pathCount, i;

    if (gpu->flags & NVG_STENCIL_STROKES)
    {
        // The stroke's base, without overlap
        sdlgpunvg__setUniforms(gpu, pass, call->uniformOffset + 1, call->image);
        if (sdlgpunvg__bind(gpu, pass, SDLGPUNVG_STENCIL_EQUAL_INCR, 1, 1, 1, &call->blend))
            for (i = 0; i < npaths; i++)
                sdlgpunvg__draw(pass, paths[i].strokeOffset, paths[i].strokeCount);

        // Its anti-aliased pixels
        sdlgpunvg__setUniforms(gpu, pass, call->uniformOffset, call->image);
        if (sdlgpunvg__bind(gpu, pass, SDLGPUNVG_STENCIL_EQUAL_KEEP, 1, 1, 1, &call->blend))
            for (i = 0; i < npaths; i++)
                sdlgpunvg__draw(pass, paths[i].strokeOffset, paths[i].strokeCount);

        // The stencil cleared again
        if (sdlgpunvg__bind(gpu, pass, SDLGPUNVG_STENCIL_ALWAYS_ZERO, 1, 0, 1, &call->blend))
            for (i = 0; i < npaths; i++)
                sdlgpunvg__draw(pass, paths[i].strokeOffset, paths[i].strokeCount);
        state->clipped = 0;
    }
    else
    {
        sdlgpunvg__setUniforms(gpu, pass, call->uniformOffset, call->image);
        if (sdlgpunvg__bind(gpu, pass, state->clipped ? SDLGPUNVG_STENCIL_EQUAL_KEEP : SDLGPUNVG_STENCIL_NONE, 1, 1, 1,
                &call->blend))
            for (i = 0; i < npaths; i++)
                sdlgpunvg__draw(pass, paths[i].strokeOffset, paths[i].strokeCount);
    }
}

static void sdlgpunvg__triangles(SDLGPUNVGcontext* gpu, SDLGPUNVGpassState* state, SDLGPUNVGcall* call)
{
    sdlgpunvg__setUniforms(gpu, state->pass, call->uniformOffset, call->image);
    if (sdlgpunvg__bind(gpu, state->pass, state->clipped ? SDLGPUNVG_STENCIL_EQUAL_KEEP : SDLGPUNVG_STENCIL_NONE, 0, 1,
            1, &call->blend))
        sdlgpunvg__draw(state->pass, call->triangleOffset, call->triangleCount);
}

static SDL_GPURenderPass* sdlgpunvg__beginPass(SDLGPUNVGcontext* gpu, int clearColor)
{
    SDL_GPUColorTargetInfo color;
    SDL_GPUDepthStencilTargetInfo depthStencil;
    SDL_GPURenderPass* pass;
    float view[4];

    memset(&color, 0, sizeof(color));
    color.texture     = gpu->color;
    color.load_op     = clearColor ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
    color.store_op    = SDL_GPU_STOREOP_STORE;
    color.clear_color = gpu->clearColor;
    memset(&depthStencil, 0, sizeof(depthStencil));
    depthStencil.texture          = gpu->depthStencil;
    depthStencil.load_op          = SDL_GPU_LOADOP_DONT_CARE;
    depthStencil.store_op         = SDL_GPU_STOREOP_DONT_CARE;
    depthStencil.stencil_load_op  = SDL_GPU_LOADOP_CLEAR;
    depthStencil.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    depthStencil.clear_stencil    = 0;
    // Its viewport is the whole target, in pixels; nanovg's view in its units
    pass = SDL_BeginGPURenderPass(gpu->cmd, &color, 1, &depthStencil);
    if (pass == NULL)
        return NULL;

    view[0] = gpu->view[0];
    view[1] = gpu->view[1];
    view[2] = view[3] = 0.0f;
    SDL_PushGPUVertexUniformData(gpu->cmd, 0, view, sizeof(view));
    return pass;
}

static int sdlgpunvg__uploadVertices(SDLGPUNVGcontext* gpu)
{
    Uint32 size = (Uint32)(gpu->nverts * sizeof(NVGvertex));
    SDL_GPUCopyPass* copy;
    SDL_GPUTransferBufferLocation source;
    SDL_GPUBufferRegion destination;
    void* mapped;

    if (size == 0)
        return 1;
    if (size > gpu->vertBufSize)
    {
        SDL_GPUBufferCreateInfo bufferInfo;
        SDL_GPUTransferBufferCreateInfo transferInfo;
        Uint32 grown = gpu->vertBufSize ? gpu->vertBufSize : 64 * 1024;
        while (grown < size)
            grown *= 2;
        if (gpu->vertBuf != NULL)
            SDL_ReleaseGPUBuffer(gpu->device, gpu->vertBuf);
        if (gpu->vertTransfer != NULL)
            SDL_ReleaseGPUTransferBuffer(gpu->device, gpu->vertTransfer);
        memset(&bufferInfo, 0, sizeof(bufferInfo));
        bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        bufferInfo.size  = grown;
        gpu->vertBuf     = SDL_CreateGPUBuffer(gpu->device, &bufferInfo);
        memset(&transferInfo, 0, sizeof(transferInfo));
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size  = grown;
        gpu->vertTransfer  = SDL_CreateGPUTransferBuffer(gpu->device, &transferInfo);
        gpu->vertBufSize   = gpu->vertBuf != NULL && gpu->vertTransfer != NULL ? grown : 0;
        if (gpu->vertBufSize == 0)
            return 0;
    }

    // Cycled: the previous frame may still read them
    mapped = SDL_MapGPUTransferBuffer(gpu->device, gpu->vertTransfer, true);
    if (mapped == NULL)
        return 0;
    memcpy(mapped, gpu->verts, size);
    SDL_UnmapGPUTransferBuffer(gpu->device, gpu->vertTransfer);

    source.transfer_buffer = gpu->vertTransfer;
    source.offset          = 0;
    destination.buffer     = gpu->vertBuf;
    destination.offset     = 0;
    destination.size       = size;
    copy                   = SDL_BeginGPUCopyPass(gpu->cmd);
    SDL_UploadToGPUBuffer(copy, &source, &destination, true);
    SDL_EndGPUCopyPass(copy);
    return 1;
}

static void sdlgpunvg__renderFlush(void* uptr)
{
    SDLGPUNVGcontext* gpu = (SDLGPUNVGcontext*)uptr;
    SDLGPUNVGpassState state;
    SDL_GPUBufferBinding binding;
    int i;

    if (gpu->cmd == NULL || gpu->color == NULL || gpu->depthStencil == NULL || (gpu->ncalls == 0 && !gpu->clear))
    {
        sdlgpunvg__renderCancel(gpu);
        return;
    }

    if (!sdlgpunvg__uploadVertices(gpu))
    {
        sdlgpunvg__renderCancel(gpu);
        return;
    }

    state.pass    = sdlgpunvg__beginPass(gpu, gpu->clear);
    state.clipped = 0;
    gpu->clear    = 0;
    if (state.pass == NULL)
    {
        sdlgpunvg__renderCancel(gpu);
        return;
    }
    if (gpu->nverts > 0)
    {
        binding.buffer = gpu->vertBuf;
        binding.offset = 0;
        SDL_BindGPUVertexBuffers(state.pass, 0, &binding, 1);
    }

    for (i = 0; i < gpu->ncalls; i++)
    {
        SDLGPUNVGcall* call = &gpu->calls[i];
        if (call->type == SDLGPUNVG_FILL)
            sdlgpunvg__fill(gpu, &state, call);
        else if (call->type == SDLGPUNVG_CONVEXFILL)
            sdlgpunvg__convexFill(gpu, &state, call, state.clipped ? SDLGPUNVG_STENCIL_EQUAL_KEEP : SDLGPUNVG_STENCIL_NONE, 1);
        else if (call->type == SDLGPUNVG_STROKE)
            sdlgpunvg__stroke(gpu, &state, call);
        else if (call->type == SDLGPUNVG_TRIANGLES)
            sdlgpunvg__triangles(gpu, &state, call);
        else if (call->type == SDLGPUNVG_CONVEXFILL_STENCIL)
        {
            // nvgStencil: its shape into the stencil (1), what follows stays
            // out of it
            SDL_SetGPUStencilReference(state.pass, 1);
            sdlgpunvg__convexFill(gpu, &state, call, SDLGPUNVG_STENCIL_ALWAYS_REPLACE, 0);
            SDL_SetGPUStencilReference(state.pass, 0);
            state.clipped = 1;
        }
        else if (call->type == SDLGPUNVG_CONVEXFILL_STENCIL_CLEAR)
        {
            // nvgStencilClear: a render pass of its own clears the stencil
            SDL_EndGPURenderPass(state.pass);
            state.pass    = sdlgpunvg__beginPass(gpu, 0);
            state.clipped = 0;
            if (state.pass == NULL)
                break;
            if (gpu->nverts > 0)
                SDL_BindGPUVertexBuffers(state.pass, 0, &binding, 1);
        }
    }
    if (state.pass != NULL)
        SDL_EndGPURenderPass(state.pass);

    sdlgpunvg__renderCancel(gpu);
}

static int sdlgpunvg__maxVertCount(const NVGpath* paths, int npaths)
{
    int i, count = 0;
    for (i = 0; i < npaths; i++)
    {
        // A fan of n vertices is a list of 3 (n - 2)
        if (paths[i].nfill > 2)
            count += 3 * (paths[i].nfill - 2);
        count += paths[i].nstroke;
    }
    return count;
}

static SDLGPUNVGcall* sdlgpunvg__allocCall(SDLGPUNVGcontext* gpu)
{
    SDLGPUNVGcall* ret = NULL;
    if (gpu->ncalls + 1 > gpu->ccalls)
    {
        int ccalls           = sdlgpunvg__maxi(gpu->ncalls + 1, 128) + gpu->ccalls / 2;
        SDLGPUNVGcall* calls = (SDLGPUNVGcall*)realloc(gpu->calls, sizeof(SDLGPUNVGcall) * ccalls);
        if (calls == NULL)
            return NULL;
        gpu->calls  = calls;
        gpu->ccalls = ccalls;
    }
    ret = &gpu->calls[gpu->ncalls++];
    memset(ret, 0, sizeof(SDLGPUNVGcall));
    return ret;
}

static int sdlgpunvg__allocPaths(SDLGPUNVGcontext* gpu, int n)
{
    int ret = 0;
    if (gpu->npaths + n > gpu->cpaths)
    {
        int cpaths           = sdlgpunvg__maxi(gpu->npaths + n, 128) + gpu->cpaths / 2;
        SDLGPUNVGpath* paths = (SDLGPUNVGpath*)realloc(gpu->paths, sizeof(SDLGPUNVGpath) * cpaths);
        if (paths == NULL)
            return -1;
        gpu->paths  = paths;
        gpu->cpaths = cpaths;
    }
    ret = gpu->npaths;
    gpu->npaths += n;
    return ret;
}

static int sdlgpunvg__allocVerts(SDLGPUNVGcontext* gpu, int n)
{
    int ret = 0;
    if (gpu->nverts + n > gpu->cverts)
    {
        int cverts       = sdlgpunvg__maxi(gpu->nverts + n, 4096) + gpu->cverts / 2;
        NVGvertex* verts = (NVGvertex*)realloc(gpu->verts, sizeof(NVGvertex) * cverts);
        if (verts == NULL)
            return -1;
        gpu->verts  = verts;
        gpu->cverts = cverts;
    }
    ret = gpu->nverts;
    gpu->nverts += n;
    return ret;
}

static int sdlgpunvg__allocFragUniforms(SDLGPUNVGcontext* gpu, int n)
{
    int ret = 0;
    if (gpu->nuniforms + n > gpu->cuniforms)
    {
        int cuniforms                   = sdlgpunvg__maxi(gpu->nuniforms + n, 128) + gpu->cuniforms / 2;
        SDLGPUNVGfragUniforms* uniforms = (SDLGPUNVGfragUniforms*)realloc(gpu->uniforms, sizeof(SDLGPUNVGfragUniforms) * cuniforms);
        if (uniforms == NULL)
            return -1;
        gpu->uniforms  = uniforms;
        gpu->cuniforms = cuniforms;
    }
    ret = gpu->nuniforms;
    gpu->nuniforms += n;
    return ret;
}

static void sdlgpunvg__vset(NVGvertex* vtx, float x, float y, float u, float v)
{
    vtx->x = x;
    vtx->y = y;
    vtx->u = u;
    vtx->v = v;
}

// A fan's vertices as a triangle list, at verts; how many were written
static int sdlgpunvg__unfoldFan(NVGvertex* verts, const NVGvertex* fan, int n)
{
    int i, count = 0;
    for (i = 1; i + 1 < n; i++)
    {
        verts[count++] = fan[0];
        verts[count++] = fan[i];
        verts[count++] = fan[i + 1];
    }
    return count;
}

static void sdlgpunvg__renderFill(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation,
    NVGscissor* scissor, float fringe, const float* bounds, const NVGpath* paths, int npaths)
{
    SDLGPUNVGcontext* gpu = (SDLGPUNVGcontext*)uptr;
    SDLGPUNVGcall* call   = sdlgpunvg__allocCall(gpu);
    SDLGPUNVGfragUniforms* frag;
    NVGvertex* quad;
    int i, maxverts, offset;

    if (call == NULL)
        return;

    call->type          = SDLGPUNVG_FILL;
    call->triangleCount = 4;
    call->pathOffset    = sdlgpunvg__allocPaths(gpu, npaths);
    if (call->pathOffset == -1)
        goto error;
    call->pathCount = npaths;
    call->image     = paint->image;
    call->blend     = sdlgpunvg__blendCompositeOperation(compositeOperation);

    if (npaths == 1 && paths[0].convex)
    {
        if (scissor->stencilFlag == NVG_STENCIL_DEFAULT)
            call->type = SDLGPUNVG_CONVEXFILL;
        else if (scissor->stencilFlag == NVG_STENCIL_ENABLE)
            call->type = SDLGPUNVG_CONVEXFILL_STENCIL;
        else if (scissor->stencilFlag == NVG_STENCIL_CLEAR)
            call->type = SDLGPUNVG_CONVEXFILL_STENCIL_CLEAR;
        call->triangleCount = 0; // no bounding quad for a convex fill
    }

    maxverts = sdlgpunvg__maxVertCount(paths, npaths) + call->triangleCount;
    offset   = sdlgpunvg__allocVerts(gpu, maxverts);
    if (offset == -1)
        goto error;

    for (i = 0; i < npaths; i++)
    {
        SDLGPUNVGpath* copy = &gpu->paths[call->pathOffset + i];
        const NVGpath* path = &paths[i];
        memset(copy, 0, sizeof(SDLGPUNVGpath));
        if (path->nfill > 2)
        {
            copy->fillOffset = offset;
            copy->fillCount  = sdlgpunvg__unfoldFan(&gpu->verts[offset], path->fill, path->nfill);
            offset += copy->fillCount;
        }
        if (path->nstroke > 0)
        {
            copy->strokeOffset = offset;
            copy->strokeCount  = path->nstroke;
            memcpy(&gpu->verts[offset], path->stroke, sizeof(NVGvertex) * path->nstroke);
            offset += path->nstroke;
        }
    }

    if (call->type == SDLGPUNVG_FILL)
    {
        // The bounding quad
        call->triangleOffset = offset;
        quad                 = &gpu->verts[call->triangleOffset];
        sdlgpunvg__vset(&quad[0], bounds[2], bounds[3], 0.5f, 1.0f);
        sdlgpunvg__vset(&quad[1], bounds[2], bounds[1], 0.5f, 1.0f);
        sdlgpunvg__vset(&quad[2], bounds[0], bounds[3], 0.5f, 1.0f);
        sdlgpunvg__vset(&quad[3], bounds[0], bounds[1], 0.5f, 1.0f);
        offset += 4;

        call->uniformOffset = sdlgpunvg__allocFragUniforms(gpu, 2);
        if (call->uniformOffset == -1)
            goto error;
        // The stencil's simple shader
        frag = &gpu->uniforms[call->uniformOffset];
        memset(frag, 0, sizeof(*frag));
        frag->strokeThr = -1.0f;
        frag->type      = SDLGPUNVG_SHADER_SIMPLE;
        // The fill's
        sdlgpunvg__convertPaint(gpu, &gpu->uniforms[call->uniformOffset + 1], paint, scissor, fringe, fringe, -1.0f);
    }
    else
    {
        call->uniformOffset = sdlgpunvg__allocFragUniforms(gpu, 1);
        if (call->uniformOffset == -1)
            goto error;
        sdlgpunvg__convertPaint(gpu, &gpu->uniforms[call->uniformOffset], paint, scissor, fringe, fringe, -1.0f);
    }
    // Fewer than allocated when a fan had under 3 vertices: the quad
    // included
    gpu->nverts = offset;
    return;

error:
    if (gpu->ncalls > 0)
        gpu->ncalls--;
}

static void sdlgpunvg__renderStroke(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation,
    NVGscissor* scissor, float fringe, float strokeWidth, const NVGpath* paths, int npaths)
{
    SDLGPUNVGcontext* gpu = (SDLGPUNVGcontext*)uptr;
    SDLGPUNVGcall* call   = sdlgpunvg__allocCall(gpu);
    int i, maxverts, offset;

    if (call == NULL)
        return;

    call->type       = SDLGPUNVG_STROKE;
    call->pathOffset = sdlgpunvg__allocPaths(gpu, npaths);
    if (call->pathOffset == -1)
        goto error;
    call->pathCount = npaths;
    call->image     = paint->image;
    call->blend     = sdlgpunvg__blendCompositeOperation(compositeOperation);

    maxverts = 0;
    for (i = 0; i < npaths; i++)
        maxverts += paths[i].nstroke;
    offset = sdlgpunvg__allocVerts(gpu, maxverts);
    if (offset == -1)
        goto error;

    for (i = 0; i < npaths; i++)
    {
        SDLGPUNVGpath* copy = &gpu->paths[call->pathOffset + i];
        const NVGpath* path = &paths[i];
        memset(copy, 0, sizeof(SDLGPUNVGpath));
        if (path->nstroke)
        {
            copy->strokeOffset = offset;
            copy->strokeCount  = path->nstroke;
            memcpy(&gpu->verts[offset], path->stroke, sizeof(NVGvertex) * path->nstroke);
            offset += path->nstroke;
        }
    }

    if (gpu->flags & NVG_STENCIL_STROKES)
    {
        call->uniformOffset = sdlgpunvg__allocFragUniforms(gpu, 2);
        if (call->uniformOffset == -1)
            goto error;
        sdlgpunvg__convertPaint(gpu, &gpu->uniforms[call->uniformOffset], paint, scissor, strokeWidth, fringe, -1.0f);
        sdlgpunvg__convertPaint(gpu, &gpu->uniforms[call->uniformOffset + 1], paint, scissor, strokeWidth, fringe,
            1.0f - 0.5f / 255.0f);
    }
    else
    {
        call->uniformOffset = sdlgpunvg__allocFragUniforms(gpu, 1);
        if (call->uniformOffset == -1)
            goto error;
        sdlgpunvg__convertPaint(gpu, &gpu->uniforms[call->uniformOffset], paint, scissor, strokeWidth, fringe, -1.0f);
    }
    return;

error:
    if (gpu->ncalls > 0)
        gpu->ncalls--;
}

static void sdlgpunvg__renderTriangles(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation,
    NVGscissor* scissor, const NVGvertex* verts, int nverts, float fringe)
{
    SDLGPUNVGcontext* gpu = (SDLGPUNVGcontext*)uptr;
    SDLGPUNVGcall* call   = sdlgpunvg__allocCall(gpu);
    SDLGPUNVGfragUniforms* frag;

    if (call == NULL)
        return;

    call->type  = SDLGPUNVG_TRIANGLES;
    call->image = paint->image;
    call->blend = sdlgpunvg__blendCompositeOperation(compositeOperation);

    call->triangleOffset = sdlgpunvg__allocVerts(gpu, nverts);
    if (call->triangleOffset == -1)
        goto error;
    call->triangleCount = nverts;
    memcpy(&gpu->verts[call->triangleOffset], verts, sizeof(NVGvertex) * nverts);

    call->uniformOffset = sdlgpunvg__allocFragUniforms(gpu, 1);
    if (call->uniformOffset == -1)
        goto error;
    frag = &gpu->uniforms[call->uniformOffset];
    sdlgpunvg__convertPaint(gpu, frag, paint, scissor, 1.0f, fringe, -1.0f);
    frag->type = SDLGPUNVG_SHADER_IMG;
    return;

error:
    if (gpu->ncalls > 0)
        gpu->ncalls--;
}

static void sdlgpunvg__renderDelete(void* uptr)
{
    SDLGPUNVGcontext* gpu = (SDLGPUNVGcontext*)uptr;
    int i;
    if (gpu == NULL)
        return;

    for (i = 0; i < gpu->npipelines; i++)
        SDL_ReleaseGPUGraphicsPipeline(gpu->device, gpu->pipelines[i].pipeline);
    for (i = 0; i < 16; i++)
        if (gpu->samplers[i] != NULL)
            SDL_ReleaseGPUSampler(gpu->device, gpu->samplers[i]);
    for (i = 0; i < gpu->ntextures; i++)
        if (gpu->textures[i].tex != NULL && (gpu->textures[i].flags & NVG_SDLGPU_IMAGE_NODELETE) == 0)
            SDL_ReleaseGPUTexture(gpu->device, gpu->textures[i].tex);
    if (gpu->vert != NULL)
        SDL_ReleaseGPUShader(gpu->device, gpu->vert);
    if (gpu->frag != NULL)
        SDL_ReleaseGPUShader(gpu->device, gpu->frag);
    if (gpu->vertBuf != NULL)
        SDL_ReleaseGPUBuffer(gpu->device, gpu->vertBuf);
    if (gpu->vertTransfer != NULL)
        SDL_ReleaseGPUTransferBuffer(gpu->device, gpu->vertTransfer);

    free(gpu->pipelines);
    free(gpu->textures);
    free(gpu->paths);
    free(gpu->verts);
    free(gpu->uniforms);
    free(gpu->calls);
    free(gpu);
}

NVGcontext* nvgCreateSDLGPU(SDL_GPUDevice* device, SDL_GPUTextureFormat colorFormat,
    SDL_GPUTextureFormat depthStencilFormat, int flags)
{
    NVGparams params;
    NVGcontext* ctx       = NULL;
    SDLGPUNVGcontext* gpu = (SDLGPUNVGcontext*)malloc(sizeof(SDLGPUNVGcontext));
    if (gpu == NULL)
        goto error;
    memset(gpu, 0, sizeof(SDLGPUNVGcontext));
    gpu->device             = device;
    gpu->colorFormat        = colorFormat;
    gpu->depthStencilFormat = depthStencilFormat;

    memset(&params, 0, sizeof(params));
    params.renderCreate         = sdlgpunvg__renderCreate;
    params.renderCreateTexture  = sdlgpunvg__renderCreateTexture;
    params.renderDeleteTexture  = sdlgpunvg__renderDeleteTexture;
    params.renderUpdateTexture  = sdlgpunvg__renderUpdateTexture;
    params.renderGetTextureSize = sdlgpunvg__renderGetTextureSize;
    params.renderViewport       = sdlgpunvg__renderViewport;
    params.renderCancel         = sdlgpunvg__renderCancel;
    params.renderFlush          = sdlgpunvg__renderFlush;
    params.renderFill           = sdlgpunvg__renderFill;
    params.renderStroke         = sdlgpunvg__renderStroke;
    params.renderTriangles      = sdlgpunvg__renderTriangles;
    params.renderDelete         = sdlgpunvg__renderDelete;
    params.userPtr              = gpu;
    params.edgeAntiAlias        = flags & NVG_ANTIALIAS ? 1 : 0;
    gpu->flags                  = flags;

    ctx = nvgCreateInternal(&params);
    if (ctx == NULL)
        goto error;
    return ctx;

error:
    // gpu is freed by nvgDeleteInternal
    if (ctx != NULL)
        nvgDeleteInternal(ctx);
    return NULL;
}

void nvgDeleteSDLGPU(NVGcontext* ctx)
{
    nvgDeleteInternal(ctx);
}

void nvgSDLGPUSetTarget(NVGcontext* ctx, SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* color, SDL_GPUTexture* depthStencil,
    const SDL_FColor* clear)
{
    SDLGPUNVGcontext* gpu = (SDLGPUNVGcontext*)nvgInternalParams(ctx)->userPtr;
    gpu->cmd              = cmd;
    gpu->color            = color;
    gpu->depthStencil     = depthStencil;
    gpu->clear            = clear != NULL;
    if (clear != NULL)
        gpu->clearColor = *clear;
}

#endif // NANOVG_SDLGPU_IMPLEMENTATION

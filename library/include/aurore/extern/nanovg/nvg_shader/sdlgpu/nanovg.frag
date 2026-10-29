// nanovg's fragment shader for SDL_GPU (nanovg_sdlgpu.h), the GL3 one's in
// Vulkan GLSL, its uniforms the GL one's array (SDLGPUNVGfragUniforms).
// tools/make_sdlgpu_shaders.py compiles it to SPIR-V and MSL, with and
// without EDGE_AA.
#version 450

layout(location = 0) in vec2 ftcoord;
layout(location = 1) in vec2 fpos;
layout(location = 0) out vec4 outColor;

// SDL_GPU: a fragment shader's textures are in set 2, its uniform buffers
// in set 3
layout(set = 2, binding = 0) uniform sampler2D tex;
layout(set = 3, binding = 0) uniform Frag
{
    vec4 frag[11];
};

#define scissorMat mat3(frag[0].xyz, frag[1].xyz, frag[2].xyz)
#define paintMat mat3(frag[3].xyz, frag[4].xyz, frag[5].xyz)
#define innerCol frag[6]
#define outerCol frag[7]
#define scissorExt frag[8].xy
#define scissorScale frag[8].zw
#define extent frag[9].xy
#define radius frag[9].z
#define feather frag[9].w
#define strokeMult frag[10].x
#define strokeThr frag[10].y
#define texType int(frag[10].z)
#define type int(frag[10].w)

float sdroundrect(vec2 pt, vec2 ext, float rad)
{
    vec2 ext2 = ext - vec2(rad, rad);
    vec2 d    = abs(pt) - ext2;
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - rad;
}

float scissorMask(vec2 p)
{
    vec2 sc = (abs((scissorMat * vec3(p, 1.0)).xy) - scissorExt);
    sc      = vec2(0.5, 0.5) - sc * scissorScale;
    return clamp(sc.x, 0.0, 1.0) * clamp(sc.y, 0.0, 1.0);
}

#ifdef EDGE_AA
// Stroke: from [0..1] to a clipped pyramid, where the slope is 1px
float strokeMask()
{
    return min(1.0, (1.0 - abs(ftcoord.x * 2.0 - 1.0)) * strokeMult) * min(1.0, ftcoord.y);
}
#endif

void main()
{
    vec4 result;
    float scissor = scissorMask(fpos);
#ifdef EDGE_AA
    float strokeAlpha = strokeMask();
    if (strokeAlpha < strokeThr)
        discard;
#else
    float strokeAlpha = 1.0;
#endif
    if (type == 0)
    {
        // Gradient
        vec2 pt    = (paintMat * vec3(fpos, 1.0)).xy;
        float d    = clamp((sdroundrect(pt, extent, radius) + feather * 0.5) / feather, 0.0, 1.0);
        vec4 color = mix(innerCol, outerCol, d);
        color *= strokeAlpha * scissor;
        result = color;
    }
    else if (type == 1)
    {
        // Image
        vec2 pt    = (paintMat * vec3(fpos, 1.0)).xy / extent;
        vec4 color = texture(tex, pt);
        if (texType == 1)
            color = vec4(color.xyz * color.w, color.w);
        if (texType == 2)
            color = vec4(color.x);
        if (texType == 3 && color.a == 1.0)
            discard;
        color *= innerCol;
        color *= strokeAlpha * scissor;
        result = color;
    }
    else if (type == 2)
    {
        // Stencil fill
        result = vec4(1, 1, 1, 1);
    }
    else
    {
        // Textured triangles
        vec4 color = texture(tex, ftcoord);
        if (texType == 1)
            color = vec4(color.xyz * color.w, color.w);
        if (texType == 2)
            color = vec4(color.x);
        color *= scissor;
        result = color * innerCol;
    }
    outColor = result;
}

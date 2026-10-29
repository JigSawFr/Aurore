// nanovg's vertex shader for SDL_GPU (nanovg_sdlgpu.h), the GL3 one's in
// Vulkan GLSL. tools/make_sdlgpu_shaders.py compiles it to SPIR-V and MSL.
#version 450

layout(location = 0) in vec2 vertex;
layout(location = 1) in vec2 tcoord;
layout(location = 0) out vec2 ftcoord;
layout(location = 1) out vec2 fpos;

// SDL_GPU: a vertex shader's uniform buffers are in set 1
layout(set = 1, binding = 0) uniform View
{
    vec4 viewSize;
};

void main()
{
    ftcoord     = tcoord;
    fpos        = vertex;
    gl_Position = vec4(2.0 * vertex.x / viewSize.x - 1.0, 1.0 - 2.0 * vertex.y / viewSize.y, 0.0, 1.0);
}

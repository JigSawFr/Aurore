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

#include <unordered_map>
#include <aurore/core/logger.hpp>
#include <aurore/platforms/ctr/ctr_video.hpp>

namespace aurore
{

namespace
{
    // nanovg without a back-end: its textures (the font atlas, pictures) kept
    // by size only, every draw call dropped
    struct NullRenderer
    {
        int nextTexture = 1;
        std::unordered_map<int, std::pair<int, int>> textures;
    };

    int renderCreate(void*)
    {
        return 1;
    }

    int renderCreateTexture(void* user, int, int w, int h, int, const unsigned char*)
    {
        auto* renderer         = static_cast<NullRenderer*>(user);
        int id                 = renderer->nextTexture++;
        renderer->textures[id] = { w, h };
        return id;
    }

    int renderDeleteTexture(void* user, int image)
    {
        return static_cast<NullRenderer*>(user)->textures.erase(image) ? 1 : 0;
    }

    int renderUpdateTexture(void* user, int image, int, int, int, int, const unsigned char*)
    {
        return static_cast<NullRenderer*>(user)->textures.count(image) ? 1 : 0;
    }

    int renderGetTextureSize(void* user, int image, int* w, int* h)
    {
        auto* renderer = static_cast<NullRenderer*>(user);
        auto it        = renderer->textures.find(image);
        if (it == renderer->textures.end())
            return 0;
        *w = it->second.first;
        *h = it->second.second;
        return 1;
    }

    void renderViewport(void*, float, float, float) { }
    void renderCancel(void*) { }
    void renderFlush(void*) { }
    void renderFill(void*, NVGpaint*, NVGcompositeOperationState, NVGscissor*, float, const float*, const NVGpath*, int) { }
    void renderStroke(void*, NVGpaint*, NVGcompositeOperationState, NVGscissor*, float, float, const NVGpath*, int) { }
    void renderTriangles(void*, NVGpaint*, NVGcompositeOperationState, NVGscissor*, const NVGvertex*, int, float) { }

    void renderDelete(void* user)
    {
        delete static_cast<NullRenderer*>(user);
    }
} // namespace

CtrVideoContext::CtrVideoContext()
{
    NVGparams params            = {};
    params.userPtr              = new NullRenderer();
    params.edgeAntiAlias        = 1;
    params.renderCreate         = renderCreate;
    params.renderCreateTexture  = renderCreateTexture;
    params.renderDeleteTexture  = renderDeleteTexture;
    params.renderUpdateTexture  = renderUpdateTexture;
    params.renderGetTextureSize = renderGetTextureSize;
    params.renderViewport       = renderViewport;
    params.renderCancel         = renderCancel;
    params.renderFlush          = renderFlush;
    params.renderFill           = renderFill;
    params.renderStroke         = renderStroke;
    params.renderTriangles      = renderTriangles;
    params.renderDelete         = renderDelete;
    this->nvgContext            = nvgCreateInternal(&params);
    if (!this->nvgContext)
        Logger::error("3DS: nanovg could not be created");
}

CtrVideoContext::~CtrVideoContext()
{
    if (this->nvgContext)
        nvgDeleteInternal(this->nvgContext);
}

} // namespace aurore

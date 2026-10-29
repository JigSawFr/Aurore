// SPDX-FileCopyrightText: 2020 Adubbz
// SPDX-FileCopyrightText: 2020-2021 Luis Scheurenbrand
// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <nanovg/framework/CMemPool.h>
#include <switch.h>

#include <deko3d.hpp>
#include <nanovg/dk_renderer.hpp>
#include <optional>
#include <aurore/core/video.hpp>

typedef Event _LibNXEvent; // "Event" alone clashes with aurore::Event

namespace aurore
{

static constexpr const unsigned FRAMEBUFFERS_COUNT = 2;

// deko3d video context
class SwitchVideoContext : public VideoContext
{
  public:
    SwitchVideoContext();
    ~SwitchVideoContext() override;

    void clear(NVGcolor color) override;
    void resetState() override;
    void beginFrame() override;
    void endFrame() override;
    void setSwapInterval(int interval) override;
    double getScaleFactor() override;
    NVGcontext* getNVGContext() override;

    void appletCallback(AppletHookType hookType);

    void recordStaticCommands();

    dk::Device getDeko3dDevice();

    dk::Queue getQueue();

    dk::Image* getFramebuffer();
    dk::Image* getDepthBuffer();

    int allocateImageIndex();
    bool freeImageIndex(int index);
    bool updateImageDescriptor(dk::CmdBuf cmdbuf, int index, dk::ImageDescriptor const& descriptor);
    void invalidateImageDescriptors(dk::CmdBuf cmdbuf);

    void queueWaitFence(DkFence* fence);

    void queueSignalFence(DkFence* fence, bool flash = false);

    void queueFlush();

  private:
    _LibNXEvent defaultDisplayResolutionChangeEvent;
    bool displayResolutionChangeEventReady = true;

    void resetFramebuffer(); // triggered by either display resolution change event or operation mode change event
    void updateWindowSize();
    void createFramebufferResources();

    void destroyFramebufferResources();

    float framebufferWidth, framebufferHeight;
    int imageSlot;

    dk::UniqueDevice device;
    dk::UniqueQueue queue;
    dk::UniqueCmdBuf cmdbuf;
    dk::UniqueSwapchain swapchain;

    dk::Image depthBuffer;
    CMemPool::Handle depthBufferHandle;

    dk::Image framebuffers[FRAMEBUFFERS_COUNT];
    CMemPool::Handle framebuffersHandles[FRAMEBUFFERS_COUNT];

    DkCmdList framebuffersCmdLists[FRAMEBUFFERS_COUNT];
    DkCmdList renderCmdlist;

    std::optional<CMemPool> imagesPool;
    std::optional<CMemPool> codePool;
    std::optional<CMemPool> dataPool;

    std::optional<nvg::DkRenderer> renderer;
    NVGcontext* nvgContext;
};

} // namespace aurore

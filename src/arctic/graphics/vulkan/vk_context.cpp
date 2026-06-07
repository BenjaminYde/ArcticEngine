#include "arctic/graphics/vulkan/vk_context.h"

#include "vk_loader.h"
#include "vk_renderloop.h"
#include "arctic/graphics/vulkan/vk_window.h"

// should only be defined once in your entire project to prevent multiple definitions of VMA functions:
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

VulkanContext::VulkanContext(std::shared_ptr<VulkanWindow> vulkanWindow)
{
    pVulkanLoader = std::make_unique<VulkanLoader>(vulkanWindow);
}

VulkanContext::~VulkanContext()
{
}

void VulkanContext::Cleanup()
{
    // cleanup vulkan
    pVulkanLoader->Cleanup();
    pVulkanLoader.reset();
}

void VulkanContext::Render()
{
    // get renderloop
    auto renderLoop = pVulkanLoader->GetRenderLoop();

    // reload swapchain when dirty
    if(renderLoop->IsSwapChainDirty())
    {
        pVulkanLoader->ReloadSwapChain();
    }

    // render
    renderLoop->Render();
}
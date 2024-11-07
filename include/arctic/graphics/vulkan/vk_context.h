#pragma once

#include <memory>

class VulkanWindow;
class VulkanLoader;
class VulkanRenderLoop;

class VulkanContext
{
public: 
    VulkanContext(VulkanWindow* vulkanWindow);
    virtual ~VulkanContext();

    void Cleanup();
    void Render();

private:
    VulkanLoader* pVulkanLoader;
};
#pragma once

#include <memory>

class VulkanWindow;
class VulkanLoader;
class VulkanRenderLoop;

class VulkanContext
{
public: 
    VulkanContext(std::shared_ptr<VulkanWindow> vulkanWindow);
    virtual ~VulkanContext();

    void Cleanup();
    void Render();

private:
    std::unique_ptr<VulkanLoader> pVulkanLoader;
};
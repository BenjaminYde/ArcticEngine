#pragma once

#include <memory>

class VulkanWindow;
class VulkanContext;

class ArcticEngine
{
public:
    ArcticEngine();
    virtual ~ArcticEngine();

    void Initialize();
    void Run();
    void Cleanup();

private:
    std::shared_ptr<VulkanWindow> pVulkanWindow;
    std::unique_ptr<VulkanContext> pVulkanContext;
};
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
    VulkanWindow* pVulkanWindow;
    VulkanContext* pVulkanContext;
};
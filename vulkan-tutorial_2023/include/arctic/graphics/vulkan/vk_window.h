#pragma once

#include <utility>
#include <vector>
#include <cstdint>

class SDL_Window;

// avoid include vulkan headers
#ifndef VULKAN_H_
    typedef struct VkInstance_T* VkInstance;
    typedef struct VkSurfaceKHR_T* VkSurfaceKHR;
#endif

class VulkanWindow
{
public:
    
    SDL_Window* GetSDLWindow();
    std::vector<const char*> GetExtensions() const;
    std::pair<uint32_t,uint32_t> GetFramebufferSize() const;

    void CreateWindow();
    void CreateSurface(const VkInstance& vkInstance, VkSurfaceKHR& vkSurface) const;
    void CleanupWindow();

private:
    
    const uint32_t WINDOW_WIDTH = 1280;
    const uint32_t WINDOW_HEIGHT = 720;
    SDL_Window* window = nullptr;
};
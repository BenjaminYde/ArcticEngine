#include "arctic/graphics/vulkan/vk_window.h"

#include <iostream>

#include <xcb/xcb.h>
#include <X11/Xlib-xcb.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

SDL_Window& VulkanWindow::GetSDLWindow()
{
    return *this->window;
}

std::vector<const char*> VulkanWindow::GetExtensions() const
{
    uint32_t extensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(this->window, &extensionCount, nullptr);
    std::vector<const char*> extensions(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions.data());
    return extensions;
}

std::pair<uint32_t, uint32_t> VulkanWindow::GetFramebufferSize() const
{
    // get window size
    int windowFrameBufferWidth;
    int windowFrameBufferHeight;
    SDL_Vulkan_GetDrawableSize(window, &windowFrameBufferWidth, &windowFrameBufferHeight);
    return std::make_pair(windowFrameBufferWidth,windowFrameBufferHeight);
}

void VulkanWindow::CreateWindow()
{
    // create SDL window
    window = SDL_CreateWindow(
        "Arctic Engine",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, 
        WINDOW_HEIGHT,
        SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    // checks if window has been created; if not, exits program
    if (window == NULL) 
    {
        std::cout << "SDL failed to initialize: " << SDL_GetError() << std::endl;
    }
  
    // pauses all SDL subsystems for a variable amount of milliseconds
    //SDL_Delay(100);
}

void VulkanWindow::CreateSurface(const VkInstance& vkInstance, VkSurfaceKHR& vkSurface) const
{
    SDL_bool result = SDL_Vulkan_CreateSurface(window, vkInstance, &vkSurface);
    if(result != SDL_TRUE)
    {
        std::cout << "error: vulkan: failed to create surface!";
        return;
    }
}

void VulkanWindow::CleanupWindow()
{
    // frees memory
    SDL_DestroyWindow(window);
  
    // Shuts down all SDL subsystems
    SDL_Quit(); 
}
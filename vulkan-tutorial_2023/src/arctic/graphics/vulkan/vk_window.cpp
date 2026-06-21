#include "arctic/graphics/vulkan/vk_window.h"

#include <iostream>

#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

SDL_Window* VulkanWindow::GetSDLWindow() {
    return this->window;
}

std::vector<const char*> VulkanWindow::GetExtensions() const {
    uint32_t                 extensionCount = 0;
    const char* const*       sdlExts        = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    std::vector<const char*> instanceExts(sdlExts, sdlExts + extensionCount);
    return instanceExts;
}

std::pair<uint32_t, uint32_t> VulkanWindow::GetFramebufferSize() const {
    // get window size
    int windowFrameBufferWidth  = 0;
    int windowFrameBufferHeight = 0;
    SDL_GetWindowSize(window, &windowFrameBufferWidth, &windowFrameBufferHeight);
    return std::make_pair(windowFrameBufferWidth, windowFrameBufferHeight);
}

void VulkanWindow::CreateWindow() {
    // create SDL window
    window = SDL_CreateWindow("Arctic Engine", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    // checks if window has been created; if not, exits program
    if (!window) {
        std::cout << "SDL failed to initialize: " << SDL_GetError() << std::endl;
    }

    // pauses all SDL subsystems for a variable amount of milliseconds
    // SDL_Delay(100);
}

void VulkanWindow::CreateSurface(const VkInstance& vkInstance, VkSurfaceKHR& vkSurface) const {
    if (!SDL_Vulkan_CreateSurface(window, vkInstance, nullptr, &vkSurface)) {
        std::cout << "error: vulkan: failed to create surface!";
        return;
    }
}

void VulkanWindow::CleanupWindow() {
    // frees memory
    SDL_DestroyWindow(window);

    // Shuts down all SDL subsystems
    SDL_Quit();
}
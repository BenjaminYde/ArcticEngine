#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan;
#endif

#include <cstdlib>
import std;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace ArcticEngine {
class Application {
  public:
    void Run() {
        VulkanCreateInstance();
        CreateWindow();
        VulkanCreateSurface();
        VulkanSelectPhysicalDevice();
        VulkanCreateLogicalDevice();
        FinalizeWindowSetup();
        MainLoop();
        Cleanup();
    }

  private:
    vk::raii::Instance   instance = nullptr;
    vk::raii::SurfaceKHR surface  = nullptr;
    SDL_Window*          window   = nullptr;

    void VulkanCreateInstance() {
        std::print("[Vulkan] Initializing...\n");

        // Create instance
        constexpr vk::ApplicationInfo APP_INFO{
            .sType              = vk::StructureType::eApplicationInfo,
            .pApplicationName   = "Hello Vulkan",
            .applicationVersion = vk::makeVersion(1, 0, 0),
            .pEngineName        = "No Engine",
            .engineVersion      = vk::makeVersion(1, 0, 0),
            .apiVersion         = vk::ApiVersion14,
        };
        vk::InstanceCreateInfo createInfo{.pApplicationInfo = &APP_INFO};
        vk::raii::Context      context;
        instance = vk::raii::Instance(context, createInfo);
    }

    void CreateWindow() {
        std::print("[Window] Initializing...\n");

        // Create SDL window
        SDL_WindowFlags windowFlags   = (SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
        constexpr int   WINDOW_WIDTH  = 1280;
        constexpr int   WINDOW_HEIGHT = 720;
        window                        = SDL_CreateWindow("Arctic Engine", WINDOW_WIDTH, WINDOW_HEIGHT, windowFlags);

        if (!window) {
            std::print("[Window] Failed to initialize: {0}\n", SDL_GetError());
            SDL_Quit();
            std::exit(EXIT_FAILURE);
        }
    }

    void VulkanCreateSurface() {
        std::print("[Vulkan] Creating surface...\n");
        VkSurfaceKHR rawSurface = nullptr;
        SDL_Vulkan_CreateSurface(window, *instance, nullptr, &rawSurface);
        surface = vk::raii::SurfaceKHR(instance, rawSurface);
    }

    void VulkanSelectPhysicalDevice() {
        std::print("[Vulkan] Selecting physical device...\n");
    }

    void VulkanCreateLogicalDevice() {
        std::print("[Vulkan] Creating logical device...\n");
    }

    void FinalizeWindowSetup() {
        std::print("[Window] Finalizing window setup...\n");
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_ShowWindow(window);
    }

    void MainLoop() {

        SDL_Event event;
        bool      running = true;
        while (running) {
            // check input
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT)
                    running = false;
            }

            // logic: todo
        }
    }

    void Cleanup() {
        std::print("Cleaning up app...\n");
        // SDL
        SDL_DestroyWindow(window);
        SDL_Quit();
        std::print("Done!\n");
    }
};
} // namespace ArcticEngine

int main() {
    auto app = ArcticEngine::Application();
    app.Run();
    return EXIT_SUCCESS;
}
// C++ standard library
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string_view>
import std;

// Vulkan C
#include <vulkan/vk_platform.h>

// Vulkan CPP
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan;
#endif

// SDL3
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

// Validation layers
const std::vector<char const*> VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

namespace ArcticEngine {
class Application {
  public:
    void Run() {
        VulkanCreateInstance();
        SetupDebugMessenger();
        CreateWindow();
        VulkanCreateSurface();
        VulkanSelectPhysicalDevice();
        VulkanCreateLogicalDevice();
        FinalizeWindowSetup();
        MainLoop();
        Cleanup();
    }

  private:
    vk::raii::Context                context;
    vk::raii::Instance               instance       = nullptr;
    vk::raii::SurfaceKHR             surface        = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
    SDL_Window*                      window         = nullptr;

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT      severity,
                                                          vk::DebugUtilsMessageTypeFlagsEXT             type,
                                                          const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                          void*                                         pUserData) {
        std::print(std::cerr, "[Vulkan] Validation layer: type {0} msg: {1}\n", to_string(type),
                   pCallbackData->pMessage);
        return vk::False;
    }

    void SetupDebugMessenger() {
        if (!ENABLE_VALIDATION_LAYERS)
            return;

        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                                            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

        vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                           vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                           vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{.messageSeverity = severityFlags,
                                                                              .messageType     = messageTypeFlags,
                                                                              .pfnUserCallback = &debugCallback};

        this->debugMessenger = this->instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    }

    void VulkanCreateInstance() {
        // Get required vulkan validation layers
        std::print("[Vulkan] Querying Vulkan validation layers...\n");

        std::vector<const char*> requiredLayers;
        if (ENABLE_VALIDATION_LAYERS) {
            std::print("[Vulkan] Enabling validation layers...\n");
            requiredLayers.assign(VALIDATION_LAYERS.begin(), VALIDATION_LAYERS.end());
        }

        // Check if the required validation layers are supported by the Vulkan implementation
        std::vector<vk::LayerProperties> layerProperties = this->context.enumerateInstanceLayerProperties();

        if (ENABLE_VALIDATION_LAYERS) {
            std::print("  Available instance layers:\n");
            for (const auto& layer : layerProperties) {
                std::print("    {}\n", std::string(layer.layerName));
            }
        }

        for (const char* requiredLayer : requiredLayers) {
            bool found = std::ranges::any_of(layerProperties, [&](const vk::LayerProperties& layer) {
                return std::string_view(layer.layerName) == requiredLayer;
            });
            if (!found) {
                std::print("[vulkan] Error: Required layer not supported: {}\n", requiredLayer);
                Cleanup();
                std::exit(EXIT_FAILURE);
            }
        }

        // Get required SDL Vulkan extensions
        std::print("[Vulkan] Querying SDL Vulkan extensions...\n");
        std::vector<const char*> requiredExtensions;

        // Add SDL extensions to the list of required extensions
        SDL_Init(SDL_INIT_VIDEO);
        uint32_t           sdlExtensionCount   = 0;
        char const* const* sdlExtensionsCStyle = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
        requiredExtensions.insert(requiredExtensions.end(), sdlExtensionsCStyle,
                                  sdlExtensionsCStyle + sdlExtensionCount);

        // Add other extensions
        requiredExtensions.emplace_back("VK_KHR_portability_enumeration"); // mandatory
        if (ENABLE_VALIDATION_LAYERS) {
            requiredExtensions.emplace_back(vk::EXTDebugUtilsExtensionName);
        }

        // Debug extension
        if (ENABLE_VALIDATION_LAYERS) {
            std::print("  Required extensions:\n");
            for (const auto& extension : requiredExtensions) {
                std::print("    {}\n", std::string_view(extension));
            }
        }

        // Check if the required extensions are supported by the Vulkan implementation
        std::vector<vk::ExtensionProperties> availableExtensions = context.enumerateInstanceExtensionProperties();
        if (ENABLE_VALIDATION_LAYERS) {
            std::print("  Available extensions:\n");
            for (const auto& extension : availableExtensions) {
                std::print("    {}\n", std::string_view(extension.extensionName));
            }
        }

        for (std::string_view requiredExtension : requiredExtensions) {
            bool isSupported =
                std::ranges::any_of(availableExtensions, [requiredExtension](const vk::ExtensionProperties& prop) {
                    return std::string_view(prop.extensionName) == requiredExtension;
                });
            if (!isSupported)
                throw std::runtime_error(std::format("Required extension not supported: {}", requiredExtension));
        }

        // Create instance
        std::print("[Vulkan] Creating instance...\n");
        vk::ApplicationInfo appInfo{.sType              = vk::StructureType::eApplicationInfo,
                                    .pApplicationName   = "Arctic Game",
                                    .applicationVersion = vk::makeVersion(1, 0, 0),
                                    .pEngineName        = "Arctic Engine",
                                    .engineVersion      = vk::makeVersion(1, 0, 0),
                                    .apiVersion         = vk::ApiVersion14};

        vk::InstanceCreateInfo createInfo{.pApplicationInfo        = &appInfo,
                                          .enabledLayerCount       = static_cast<uint32_t>(requiredLayers.size()),
                                          .ppEnabledLayerNames     = requiredLayers.data(),
                                          .enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.size()),
                                          .ppEnabledExtensionNames = requiredExtensions.data(),
                                          .pNext};

        if (ENABLE_VALIDATION_LAYERS) {
            createInfo.setPNext(&debugCreateInfo);
        }

        try {
            this->instance = vk::raii::Instance(context, createInfo);
        } catch (const std::exception& err) {
            std::print("[vulkan] Error: Instance creation failed: {}\n", err.what());
            Cleanup();
            std::exit(EXIT_FAILURE);
        }
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
            Cleanup();
            std::exit(EXIT_FAILURE);
        }
    }

    void VulkanCreateSurface() {
        std::print("[Vulkan] Creating surface...\n");
        VkSurfaceKHR rawSurface = nullptr;
        SDL_Vulkan_CreateSurface(window, *this->instance, nullptr, &rawSurface);
        this->surface = vk::raii::SurfaceKHR(this->instance, rawSurface);
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
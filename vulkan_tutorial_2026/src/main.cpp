// C++ standard library
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
import std;

// Vulkan C
#include <vulkan/vk_platform.h>
// #include <vulkan/vulkan.hpp>
// #include <vulkan/vulkan_enums.hpp>

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
        VulkanSelectPhysicalDevice();
        VulkanCreateLogicalDevice();
        CreateWindow();
        VulkanCreateSurface();
        ShowWindow();
        MainLoop();
        Cleanup();
    }

  private:
    vk::raii::Context                m_context;
    vk::raii::Instance               m_instance       = nullptr;
    vk::raii::PhysicalDevice         m_physicalDevice = nullptr;
    vk::raii::SurfaceKHR             m_surface        = nullptr;
    vk::raii::DebugUtilsMessengerEXT m_debugMessenger = nullptr;
    SDL_Window*                      m_window         = nullptr;

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT /*severity*/,
                                                          vk::DebugUtilsMessageTypeFlagsEXT             type,
                                                          const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                          void* /*pUserData*/) {
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
                                                                              .pfnUserCallback = &DebugCallback};

        this->m_debugMessenger = this->m_instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
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
        std::vector<vk::LayerProperties> availableLayers = this->m_context.enumerateInstanceLayerProperties();

        if (ENABLE_VALIDATION_LAYERS) {
            std::print("  Available instance layers:\n");
            for (const auto& layer : availableLayers) {
                std::print("    {}\n", layer.layerName.data());
            }
        }

        for (const char* requiredLayer : requiredLayers) {
            bool found = std::ranges::any_of(availableLayers, [&](const vk::LayerProperties& layer) {
                return std::strcmp(layer.layerName.data(), requiredLayer);
            });
            if (!found) {
                std::print(std::cerr, "[vulkan] Error: Required layer not supported: {}\n", requiredLayer);
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
                std::print("    {}\n", extension);
            }
        }

        // Check if the required extensions are supported by the Vulkan implementation
        std::vector<vk::ExtensionProperties> availableExtensions = m_context.enumerateInstanceExtensionProperties();
        if (ENABLE_VALIDATION_LAYERS) {
            std::print("  Available extensions:\n");
            for (const auto& extension : availableExtensions) {
                std::print("    {}\n", extension.extensionName.data());
            }
        }

        for (const auto* requiredExtension : requiredExtensions) {
            bool isSupported =
                std::ranges::any_of(availableExtensions, [requiredExtension](const vk::ExtensionProperties& prop) {
                    return std::strcmp(prop.extensionName, requiredExtension);
                });
            if (!isSupported) {
                std::print(std::cerr, "[vulkan] Error: Required layer not supported: {}\n", requiredExtension);
                Cleanup();
                std::exit(EXIT_FAILURE);
            }
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
                                          .ppEnabledExtensionNames = requiredExtensions.data()};

        try {
            this->m_instance = vk::raii::Instance(m_context, createInfo);
        } catch (const std::exception& err) {
            std::print(std::cerr, "[vulkan] Error: Instance creation failed: {}\n", err.what());
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
        m_window                      = SDL_CreateWindow("Arctic Engine", WINDOW_WIDTH, WINDOW_HEIGHT, windowFlags);

        if (!m_window) {
            std::print(std::cerr, "[Window] Failed to initialize: {0}\n", SDL_GetError());
            Cleanup();
            std::exit(EXIT_FAILURE);
        }
    }

    void VulkanCreateSurface() {
        std::print("[Vulkan] Creating surface...\n");
        VkSurfaceKHR rawSurface = nullptr;
        SDL_Vulkan_CreateSurface(m_window, *this->m_instance, nullptr, &rawSurface);
        this->m_surface = vk::raii::SurfaceKHR(this->m_instance, rawSurface);
    }

    bool IsDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice) {
        // Check properties
        auto deviceProperties = physicalDevice.getProperties();
        if (deviceProperties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu &&
            deviceProperties.apiVersion < vk::ApiVersion13)
            return false;

        // Check features
        auto deviceFeatures = physicalDevice.getFeatures();
        if (!deviceFeatures.geometryShader)
            return false;

        // Check queue families
        auto queueFamilies   = physicalDevice.getQueueFamilyProperties();
        bool supportGraphics = std::ranges::any_of(queueFamilies, [](auto const& qfp) {
            return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
        });
        if (!supportGraphics)
            return false;

        // Check device extensions
        std::vector<const char*> requiredDeviceExtensions  = {vk::KHRSwapchainExtensionName};
        auto                     availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();

        for (const auto* requiredDeviceExtension : requiredDeviceExtensions) {
            bool isSupported = std::ranges::any_of(availableDeviceExtensions, [requiredDeviceExtension](auto const& e) {
                return std::strcmp(e.extensionName, requiredDeviceExtension);
            });
            if (!isSupported)
                return false;
        }

        // Check features 2
        auto features =
            physicalDevice.template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features,
                                                 vk::PhysicalDeviceVulkan13Features,
                                                 vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
        bool supportsRequiredFeatures =
            features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
            features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
            features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;
        return supportsRequiredFeatures;
    }

    void VulkanSelectPhysicalDevice() {
        std::print("[Vulkan] Selecting physical device...\n");

        // Get physical devices
        std::vector<vk::raii::PhysicalDevice> physicalDevices = m_instance.enumeratePhysicalDevices();

        if (physicalDevices.empty()) {
            std::print(std::cerr, "[Vulkan] Failed to find GPU's with Vulkan support!\n");
            Cleanup();
            std::exit(EXIT_FAILURE);
        }

        // Find suitable device
        for (const vk::raii::PhysicalDevice& physicalDevice : physicalDevices) {
            if (IsDeviceSuitable(physicalDevice)) {
                this->m_physicalDevice = physicalDevice;
                break;
            }
        }
        if (this->m_physicalDevice == nullptr) {
            std::print(std::cerr, "[Vulkan] Found GPU's but none that are suitable!\n");
            Cleanup();
            std::exit(EXIT_FAILURE);
        }
    }

    void VulkanCreateLogicalDevice() {
        std::print("[Vulkan] Creating logical device...\n");
    }

    void ShowWindow() {
        std::print("[Window] Finalizing window setup...\n");
        SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_ShowWindow(m_window);
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
        SDL_DestroyWindow(m_window);
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
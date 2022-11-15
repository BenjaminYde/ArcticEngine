//
// Created by Benjamin on 14/11/2022.
//

#ifndef ARCTIC_VULKANLOADER_H
#define ARCTIC_VULKANLOADER_H

#include <vector>
#include <optional>
#include <set>
#include <vulkan/vulkan_core.h>

class GLFWwindow;

class VulkanLoader
{
public:
    void Load();
    void Cleanup();

    GLFWwindow* GetWindow() {
        return window;
    }

private:
    // glfw
    const uint32_t WINDOW_WIDTH = 1280;
    const uint32_t WINDOW_HEIGHT = 720;
    GLFWwindow* window = nullptr;

    // vulkan
    VkInstance vkInstance = nullptr;

    VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
    VkDevice vkDevice = VK_NULL_HANDLE;

    VkSurfaceKHR vkSurface;
    VkSwapchainKHR vkSwapChain;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    const std::vector<const char*> requiredDeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool IsComplete()
        {
            return  graphicsFamily.has_value() &&
                    presentFamily.has_value();
        }
    };

    struct SwapChainDeviceSupport
    {
        VkSurfaceCapabilitiesKHR capabilities; // image width/height, min/max images, ...
        std::vector<VkSurfaceFormatKHR> surfaceFormats; // pixel format, color space, ...
        std::vector<VkPresentModeKHR> presentModes; // FIFO, Mailbox, ...
    };

    struct SwapChainData
    {
        VkFormat imageFormat;
        VkExtent2D extent;
    };
    SwapChainData swapChainData;

    // .. debugging
    const bool enableValidationLayers = false;

    VkDebugUtilsMessengerEXT debugMessenger;
    const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation" // all the useful standard validation is bundled into a layer included in the SDK
    };


    void vulkanCreateInstance();
    void vulkanLoadDebugMessenger();
    void vulkanLoadSurface();
    void vulkanLoadPhysicalDevice();
    void vulkanCreateLogicalDevice();
    void vulkanCreateSwapChain();
    void vulkanCreateImageViews();
    void vulkanCreatePipeline();

    // devices
    std::vector<const char*> vulkanGetRequiredExtensions();
    bool isVkDeviceSuitable(const VkPhysicalDevice& device,
                            VkPhysicalDeviceProperties deviceProperties,
                            VkPhysicalDeviceFeatures deviceFeatures,
                            QueueFamilyIndices queueFamilyIndices);
    QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& device);
    bool findRequiredDeviceExtensions(const VkPhysicalDevice& device);

    // sawp chain
    SwapChainDeviceSupport querySwapChainSupport(const VkPhysicalDevice& device);
    VkSurfaceFormatKHR selectSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR selectSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D selectSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    // validation layers
    void vulkanDestroyDebugMessenger();
    bool vulkanFoundValidationLayers();
    VkResult vulkanCreateDebugUtilsMessengerEXT(VkInstance instance,
                                                const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                const VkAllocationCallbacks* pAllocator,
                                                VkDebugUtilsMessengerEXT* pDebugMessenger);
    void vulkanPopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void vulkanDestroyDebugUtilsMessengerEXT(VkInstance instance,
                                             VkDebugUtilsMessengerEXT debugMessenger,
                                             const VkAllocationCallbacks* pAllocator);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData);
};


#endif //ARCTIC_VULKANLOADER_H
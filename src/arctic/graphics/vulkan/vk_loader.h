#pragma once

#include <vector>
#include <optional>
#include <set>
#include <vulkan/vulkan_core.h>
#include <memory>

class VulkanWindow;
class VulkanRenderPipeline;
class VulkanSwapChain;
class VulkanRenderLoop;
class VulkanMemoryHandler;

class VulkanLoader
{
public:
    VulkanLoader(std::shared_ptr<VulkanWindow> vulkanWindow);
    void Cleanup();

    std::shared_ptr<VulkanRenderLoop> GetRenderLoop();
    void ReloadSwapChain();

private:
    // vulkan
    VkInstance vkInstance = nullptr;

    VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
    VkDevice vkDevice = VK_NULL_HANDLE;

    VkQueue vkGraphicsQueue;
    VkQueue vkTransferQueue;
    VkQueue vkPresentQueue;

    const std::vector<const char*> requiredDeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        std::optional<uint32_t> transferFamily;

        bool IsComplete()
        {
            return  graphicsFamily.has_value() &&
                    presentFamily.has_value() &&
                    transferFamily.has_value();
        }
    };

    VkSurfaceKHR vkSurface;
    std::shared_ptr<VulkanSwapChain> pSwapchain;
    std::shared_ptr<VulkanRenderPipeline> pRenderPipeline;
    std::shared_ptr<VulkanRenderLoop> pRenderLoop;
    std::shared_ptr<VulkanMemoryHandler> pMemoryHandler;

    void vulkanCreateInstance(const VulkanWindow & vulkanWindow);
    void vulkanLoadDebugMessenger();
    
    void vulkanLoadPhysicalDevice(const VkInstance& instance, const VkSurfaceKHR& surface);

    void vulkanCreateLogicalDevice(const VkPhysicalDevice & vkPhysicalDevice, QueueFamilyIndices indices);

    // devices
    std::vector<const char*> vulkanGetRequiredExtensions(const VulkanWindow & vulkanWindow);
    bool isVkDeviceSuitable(const VkPhysicalDevice& device,
                            const VkSurfaceKHR & vkSurface,
                            VkPhysicalDeviceProperties deviceProperties,
                            VkPhysicalDeviceFeatures deviceFeatures,
                            QueueFamilyIndices queueFamilyIndices) const;
    QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& device, const VkSurfaceKHR & surface);
    bool findRequiredDeviceExtensions(const VkPhysicalDevice& device) const;

    // validation layers
    const bool enableValidationLayers = false;

    VkDebugUtilsMessengerEXT debugMessenger;
    const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation" // all the useful standard validation is bundled into a layer included in the SDK
    };

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

    // memory
    VkPhysicalDeviceMemoryProperties memProperties;
};
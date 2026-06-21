#pragma once

#include <vector>
#include <memory>
#include <vulkan/vulkan_core.h>

struct SwapChainDeviceSupport
{
    VkSurfaceCapabilitiesKHR capabilities;          // image width/height, min/max images, ...
    std::vector<VkSurfaceFormatKHR> surfaceFormats; // pixel format, color space, ...
    std::vector<VkPresentModeKHR> presentModes;     // FIFO, Mailbox, ...
};

struct SwapChainData
{
    uint32_t imageCount;
    VkFormat imageFormat;
    VkExtent2D extent;
};

class VulkanWindow;

class VulkanSwapChain
{
public:
    VulkanSwapChain(const VkDevice& vkDevice, const VkPhysicalDevice& vkPhysicalDevice, const VkSurfaceKHR& vkSurface, const std::shared_ptr<VulkanWindow>& window);
    void CleanUp();

    SwapChainDeviceSupport QuerySwapChainSupport(const VkPhysicalDevice &device, const VkSurfaceKHR &vkSurface) const;
    bool CreateSwapChain();

    const SwapChainData GetData();
    const VkSwapchainKHR &GetSwapChain();
    const std::vector<VkImageView> &GetImageViews();

private:
    VkSurfaceFormatKHR selectSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR selectSwapChainPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D selectSwapChainExtent(const VulkanWindow& window, const VkSurfaceCapabilitiesKHR &capabilities);
    void createImageViews(const VkSwapchainKHR& vkSwapChain, const SwapChainData& swapChainData, std::vector<VkImage>* pSwapChainImages, std::vector<VkImageView>* pSwapChainImageViews);

    VkDevice vkDevice;
    VkPhysicalDevice vkPhysicalDevice;

    SwapChainData swapChainData;
    VkSurfaceKHR vkSurface;
    VkSwapchainKHR vkSwapChain;
    std::shared_ptr<VulkanWindow> window;

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
};
#include "vk_swapchain.h"
#include "arctic/graphics/vulkan/vk_window.h"
#include <iostream>
#include <algorithm>

VulkanSwapChain::VulkanSwapChain(const VkDevice &vkDevice, const VkPhysicalDevice &vkPhysicalDevice, const VkSurfaceKHR &vkSurface, const std::shared_ptr<VulkanWindow>& window)
:
    vkDevice(vkDevice),
    vkPhysicalDevice(vkPhysicalDevice),
    vkSurface(vkSurface),
    window(window)
{
}

SwapChainDeviceSupport VulkanSwapChain::QuerySwapChainSupport(const VkPhysicalDevice &device, const VkSurfaceKHR &vkSurface) const
{
    SwapChainDeviceSupport details;

    // get capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vkSurface, &details.capabilities);

    // get surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, vkSurface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.surfaceFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, vkSurface, &formatCount, details.surfaceFormats.data());
    }

    // get present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, vkSurface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, vkSurface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

bool VulkanSwapChain::CreateSwapChain()
{
    SwapChainDeviceSupport swapChainSupport = QuerySwapChainSupport(this->vkPhysicalDevice, this->vkSurface);

    // select best settings from query
    VkSurfaceFormatKHR surfaceFormat = selectSwapChainSurfaceFormat(swapChainSupport.surfaceFormats);
    VkPresentModeKHR presentMode = selectSwapChainPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = selectSwapChainExtent(*this->window, swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1; // make sure to have al least 2 images
    imageCount = std::clamp(imageCount, static_cast<uint32_t>(1), swapChainSupport.capabilities.maxImageCount);
    imageCount = 3; // set to triple buffering

    // create swap chain info
    // .. default data
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = vkSurface;

    // .. image data
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;

    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // specified operation usage: no special usage (just render directly), no-post-processing...

    // .. queue family data
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0; // optional
    createInfo.pQueueFamilyIndices = nullptr; // optional

    // .. other data
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform; // avoid special transform by setting default
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // ignore window blending

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    // create swap chain
    VkResult result = vkCreateSwapchainKHR(this->vkDevice, &createInfo, nullptr, &this->vkSwapChain);
    if (result != VK_SUCCESS)
    {
        std::cout << "error: vulkan: failed to create swap chain!";
        return false;
    }

    // set swapchain data
    this->swapChainData = {};
    this->swapChainData.imageFormat = surfaceFormat.format;
    this->swapChainData.extent = extent;
    this->swapChainData.imageCount = imageCount;

    // create views
    createImageViews(this->vkSwapChain, this->swapChainData, &this->swapChainImages, &this->swapChainImageViews);
    return true;
}


VkSurfaceFormatKHR VulkanSwapChain::selectSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
    // loop over available formats
    for (const auto& availableFormat : availableFormats)
    {
        // return format when met requirements
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;
    }

    // return first format if none found
    return availableFormats[0];
}

VkPresentModeKHR VulkanSwapChain::selectSwapChainPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
{
    // loop over available modes
    for (const auto& availablePresentMode : availablePresentModes)
    {
        // return mode when met requirements
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentMode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D VulkanSwapChain::selectSwapChainExtent(const VulkanWindow& window, const VkSurfaceCapabilitiesKHR & capabilities)
{
    // get window size
    auto framebufferSize = window.GetFramebufferSize();

    // create vulkan extent
    VkExtent2D extent = {};
    extent.width = std::clamp(static_cast<uint32_t>(framebufferSize.first), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height = std::clamp(static_cast<uint32_t>(framebufferSize.second), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return extent;
}

void VulkanSwapChain::CleanUp()
{
    // cleanup images
    for(auto & imageView : this->swapChainImageViews)
        vkDestroyImageView(this->vkDevice, imageView, nullptr);

    // cleanup swapchain
    vkDestroySwapchainKHR(this->vkDevice, this->vkSwapChain, nullptr);
}

const SwapChainData VulkanSwapChain::GetData()
{
    return this->swapChainData;
}

const VkSwapchainKHR &VulkanSwapChain::GetSwapChain()
{
    return this->vkSwapChain;
}

const std::vector<VkImageView> &VulkanSwapChain::GetImageViews()
{
    return this->swapChainImageViews;
}

void VulkanSwapChain::createImageViews(
    const VkSwapchainKHR& vkSwapChain,
    const SwapChainData& swapChainData,
    std::vector<VkImage>* pSwapChainImages,
    std::vector<VkImageView>* pSwapChainImageViews)
{
    // get image handles
    uint32_t imageCount = swapChainData.imageCount;
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(vkDevice, vkSwapChain, &imageCount, swapChainImages.data());

    // resize views from created images
    swapChainImageViews.resize(swapChainImages.size());

    // create views
    for (size_t i = 0; i < swapChainImages.size(); ++i)
    {
        // create view info
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];

        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainData.imageFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        // create view
        VkResult result = vkCreateImageView(vkDevice, &createInfo, nullptr, &swapChainImageViews[i]);
        if (result != VK_SUCCESS)
        {
            std::cout << "error: vulkan: failed to create swap chain image view from image!";
            return;
        }
    }
}
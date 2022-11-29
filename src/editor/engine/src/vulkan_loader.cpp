#include "vulkan_loader.h"
#include "utilities/file_utility.h"
#include "utilities/Application.h"

#ifdef WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <algorithm>
#include <limits>
#include <iostream>
#include <format>

void VulkanLoader::vulkanCreateInstance()
{
    // create app info
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // create vk instance create info
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.pNext = nullptr;

    // apply validation layers
    if(enableValidationLayers)
    {
        // set layers
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        // set debug messenger info
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        vulkanPopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    }

    // set extensions
    auto extensions = vulkanGetRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // create vk instance
    VkResult result = vkCreateInstance(&createInfo, nullptr, &vkInstance);
    if( result != VK_SUCCESS)
    {
        std::cout << "error: vulkan: failed to create instance!";
        return;
    }
}

std::vector<const char*> VulkanLoader::vulkanGetRequiredExtensions()
{
    // create empty extensions
    std::vector<const char*> extensions;

    // get glfw extensions
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for(int i=0; i<glfwExtensionCount; ++i)
    {
        const char* extension = glfwExtensions[i];
        extensions.push_back(extension);
    }

    // get validation extension
    if (enableValidationLayers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    // debug extensions
    bool debugExtentions = false;
    if(debugExtentions)
    {
        std::cout << "info: vulkan: available extensions: " << std::endl;
        for( const auto& extension : extensions)
        {
            std::cout << "\t" << extension << std::endl;
        }
    }

    return extensions;
}

#pragma region vulkan_devices

void VulkanLoader::vulkanLoadPhysicalDevice()
{
    // get available physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);

    if(deviceCount == 0)
    {
        std::cout << "error: vulkan: did not find physical device!";
        return;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());

    // find suitable device
    // todo: idea: could add score implementation (more features = better score), select device with highest score
    vkPhysicalDevice = VK_NULL_HANDLE;

    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    QueueFamilyIndices queueFamilyIndices;

    for(auto & device : devices)
    {
        // get data of device
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        queueFamilyIndices = findQueueFamilies(device);

        // break loop when found suitable device
        if(isVkDeviceSuitable(device, deviceProperties, deviceFeatures, queueFamilyIndices))
        {
            vkPhysicalDevice = device;
            break;
        }
    }

    // final check if device is valid
    if(vkPhysicalDevice == VK_NULL_HANDLE)
    {
        std::cout << "error: vulkan: did not find suitable physical device!";
        return;
    }
}

void VulkanLoader::vulkanCreateLogicalDevice()
{
    // create device queue infos
    // >> create set of queue families (re-use queue families instead of creating duplicates)
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    QueueFamilyIndices indices = findQueueFamilies(vkPhysicalDevice);
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for(uint32_t queueFamily : uniqueQueueFamilies)
    {
        // create device queue create info
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        // add create info
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // create device features
    // >> currently empty
    VkPhysicalDeviceFeatures deviceFeatures{};

    // create device info
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

    // create device
    VkResult result = vkCreateDevice(vkPhysicalDevice, &createInfo, nullptr, &vkDevice);
    if(result != VK_SUCCESS)
    {
        std::cout << "error: vulkan: failed to create logical device!";
        return;
    }
}

bool VulkanLoader::isVkDeviceSuitable(
        const VkPhysicalDevice & device,
        VkPhysicalDeviceProperties deviceProperties,
        VkPhysicalDeviceFeatures deviceFeatures,
        QueueFamilyIndices queueFamilyIndices)
{
    // check device properties
    if(deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        return false;

    // check device features
    if(!deviceFeatures.geometryShader)
        return false;

    // check if queue families are complete
    if(!queueFamilyIndices.IsComplete())
        return false;

    // try find device extensions
    bool foundDeviceExtensions = findRequiredDeviceExtensions(device);
    if(!foundDeviceExtensions)
        return false;

    // check if swap chain is valid
    // >> see device & surface
    SwapChainDeviceSupport swapChainSupport = querySwapChainSupport(device);
    bool isSwapChainValid = !swapChainSupport.surfaceFormats.empty() && !swapChainSupport.presentModes.empty();
    if(!isSwapChainValid)
        return false;

    return true;
}

VulkanLoader::QueueFamilyIndices VulkanLoader::findQueueFamilies(const VkPhysicalDevice & device)
{
    // get queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    // find suitable families
    QueueFamilyIndices queueFamilyIndices{};
    uint32_t familyIndex = 0;
    for(const auto& queueFamily : queueFamilies)
    {
        // set graphics family
        if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            queueFamilyIndices.graphicsFamily = familyIndex;

        // set present family
        VkBool32 isPresentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, familyIndex, vkSurface, &isPresentSupport);
        if(isPresentSupport)
            queueFamilyIndices.presentFamily = familyIndex;

        ++familyIndex;
    }
    return queueFamilyIndices;
}

bool VulkanLoader::findRequiredDeviceExtensions(const VkPhysicalDevice & device)
{
    // get available device extensions
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    if(extensionCount == 0)
        return false;

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    // check if available extensions met requirements
    std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());
    for(const auto & extension : availableExtensions)
        requiredExtensions.erase(extension.extensionName);

    return requiredExtensions.empty();
}

#pragma endregion vulkan_devices

#pragma region vulkan_presentation

void VulkanLoader::vulkanCreateSwapChain()
{
    // query device support
    SwapChainDeviceSupport swapChainSupport = querySwapChainSupport(vkPhysicalDevice);

    // select best settings from query
    VkSurfaceFormatKHR surfaceFormat = selectSwapChainSurfaceFormat(swapChainSupport.surfaceFormats);
    VkPresentModeKHR presentMode = selectSwapChainPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = selectSwapChainExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1; // make sure to have al least 2 images
    imageCount = std::clamp(imageCount, static_cast<uint32_t>(1), swapChainSupport.capabilities.maxImageCount);

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
    QueueFamilyIndices indices = findQueueFamilies(vkPhysicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    bool isQueueFamilyShared = indices.graphicsFamily == indices.presentFamily;
    if(isQueueFamilyShared)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // avoid extra ownership code
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }

    // .. other data
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform; // avoid special transform by setting default
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // ignore window blending

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    // create swap chain
    VkResult result = vkCreateSwapchainKHR(vkDevice, &createInfo, nullptr, &vkSwapChain);
    if (result != VK_SUCCESS)
    {
        std::cout << "error: vulkan: failed to create swap chain!";
        return;
    }

    swapChainData = {};
    swapChainData.imageFormat = surfaceFormat.format;
    swapChainData.extent = extent;

    // get image handles
    vkGetSwapchainImagesKHR(vkDevice, vkSwapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(vkDevice, vkSwapChain, &imageCount, swapChainImages.data());
}

void VulkanLoader::vulkanCreateImageViews()
{
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

VulkanLoader::SwapChainDeviceSupport VulkanLoader::querySwapChainSupport(const VkPhysicalDevice & device)
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

VkSurfaceFormatKHR VulkanLoader::selectSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> & availableFormats)
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

VkPresentModeKHR VulkanLoader::selectSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
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

VkExtent2D VulkanLoader::selectSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    // get window size
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    // create vulkan extent
    VkExtent2D extent = {};
    extent.width = std::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height = std::clamp(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return extent;
}

#pragma endregion vulkan_presentation

#pragma region vulkan_pipeline

void VulkanLoader::vulkanCreatePipeline()
{
    // read vertex shader
    std::vector<char> fileVert;
    std::string pathVert = std::format("{}/shaders/{}", Application::AssetsPath, "vert.spv");
    FileUtility::ReadBinaryFile(pathVert, fileVert);
    VkShaderModule shaderModuleVert;
    vulkanCreateShaderModule(fileVert, shaderModuleVert);

    // read frag shader
    std::vector<char> fileFrag;
    std::string pathFrag= std::format("{}/shaders/{}",Application::AssetsPath, "frag.spv");
    FileUtility::ReadBinaryFile(pathFrag, fileFrag);
    VkShaderModule shaderModuleFrag;
    vulkanCreateShaderModule(fileFrag, shaderModuleFrag);

    // create pipeline: vertex
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = shaderModuleVert;
    vertShaderStageInfo.pName = "main";

    // create pipeline: frag
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = shaderModuleFrag;
    fragShaderStageInfo.pName = "main";

    // combine pipelines
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // cleanup shaders
    vkDestroyShaderModule(vkDevice, shaderModuleVert, nullptr);
    vkDestroyShaderModule(vkDevice, shaderModuleFrag, nullptr);
}

bool VulkanLoader::vulkanCreateShaderModule(const std::vector<char>& code, VkShaderModule& shaderModule)
{
    // create shader create info
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    // create shader
    VkResult result = vkCreateShaderModule(vkDevice, &createInfo, nullptr, &shaderModule);
    return result == VK_SUCCESS;
}

#pragma endregion vulkan_pipeline

#pragma region vulkan_validation

void VulkanLoader::vulkanLoadDebugMessenger()
{
    // return when no validation layers
    if (!enableValidationLayers)
        return;

    // create debug messenger info
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    vulkanPopulateDebugMessengerCreateInfo(debugCreateInfo);

    // create debug messenger
    auto result = vulkanCreateDebugUtilsMessengerEXT(vkInstance, &debugCreateInfo, nullptr, &debugMessenger);
    if( result != VK_SUCCESS)
    {
        std::cout << "error: vulkan: failed to create debug messenger!";
        return;
    }
}

void VulkanLoader::vulkanDestroyDebugMessenger()
{
    if (enableValidationLayers)
        vulkanDestroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, nullptr);
}

bool VulkanLoader::vulkanFoundValidationLayers()
{
    // get available layers
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // check if all layers are present
    // >> return true
    for(const auto validationLayer : validationLayers)
    {
        bool layerFound = false;
        for(const auto & availableLayer : availableLayers)
        {
            if(strcmp(validationLayer, availableLayer.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if(!layerFound)
            return false;
    }
    return true;
}

VkResult VulkanLoader::vulkanCreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void VulkanLoader::vulkanDestroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
        func(instance, debugMessenger, pAllocator);
}

void VulkanLoader::vulkanPopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanLoader::debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

#pragma endregion vulkan_validation

void VulkanLoader::vulkanLoadSurface()
{
    // create native surface
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = glfwGetWin32Window(window);
    createInfo.hinstance = GetModuleHandle(nullptr);

    VkResult resultWindows = vkCreateWin32SurfaceKHR(vkInstance, &createInfo, nullptr, &vkSurface);
    if(resultWindows != VK_SUCCESS)
    {
        std::cout << "error: vulkan: failed to create window 32 surface!";
        return;
    }

    // create glfw surface from native surface
    VkResult resultGlfw = glfwCreateWindowSurface(vkInstance, window, nullptr, &vkSurface);
    if(resultGlfw != VK_SUCCESS)
    {
        std::cout << "error: vulkan: failed to create glfw window surface!";
        return;
    }
}

void VulkanLoader::Load()
{
    // init glfw
    glfwInit();

    // set hints
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // create window
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);

    // check validation layers
    if(enableValidationLayers && !vulkanFoundValidationLayers())
    {
        std::cout << "error: vulkan: validation layers requested, but not available!";
        return;
    }

    // create instance
    vulkanCreateInstance();
    vulkanLoadDebugMessenger();
    vulkanLoadSurface();
    vulkanLoadPhysicalDevice();
    vulkanCreateLogicalDevice();
    vulkanCreateSwapChain();
    vulkanCreateImageViews();
    vulkanCreatePipeline();
}

void VulkanLoader::Cleanup()
{
    // vulkan
    for(auto & imageView : swapChainImageViews)
    {
        vkDestroyImageView(vkDevice, imageView, nullptr);
    }
    vkDestroySwapchainKHR(vkDevice, vkSwapChain, nullptr);
    vkDestroyDevice(vkDevice, nullptr);
    vulkanDestroyDebugMessenger();
    vkDestroySurfaceKHR(vkInstance, vkSurface, nullptr);
    vkDestroyInstance(vkInstance, nullptr);

    // glfw
    glfwDestroyWindow(window);
    glfwTerminate();
}

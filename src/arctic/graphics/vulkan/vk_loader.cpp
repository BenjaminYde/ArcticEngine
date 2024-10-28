#include "vk_loader.h"

#include "arctic/core/utilities/file_utility.h"
#include "arctic/core/utilities/application.h"

#include <iostream>
#include <fmt/core.h>

#include "arctic/graphics/vulkan/vk_window.h"
#include "vk_renderpipeline.h"
#include "vk_renderloop.h"
#include "vk_swapchain.h"
#include "vk_memory_handler.h"

std::shared_ptr<VulkanRenderLoop> VulkanLoader::GetRenderLoop()
{
    return pRenderLoop;
}

void VulkanLoader::ReloadSwapChain()
{
    // re-create swapchain and re-load dependencies on swapchain data
    this->pSwapchain->CleanUp();
    this->pSwapchain->CreateSwapChain();

    this->pRenderPipeline->Load(
        this->pSwapchain->GetData(), 
        this->pSwapchain->GetImageViews());
}

VulkanLoader::VulkanLoader(std::shared_ptr<VulkanWindow> vulkanWindow)
{
    // check validation layers
    if(enableValidationLayers && !vulkanFoundValidationLayers())
    {
        std::cout << "error: vulkan: validation layers requested, but not available!";
        return;
    }

    vulkanCreateInstance(*vulkanWindow.get());
    vulkanLoadDebugMessenger();

    vulkanWindow->CreateSurface(vkInstance, vkSurface);
    vulkanLoadPhysicalDevice(vkInstance, vkSurface);
    
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(vkPhysicalDevice, vkSurface);
    vulkanCreateLogicalDevice(vkPhysicalDevice, queueFamilyIndices);

    this->pSwapchain = std::make_shared<VulkanSwapChain>(vkDevice, vkPhysicalDevice, vkSurface, vulkanWindow);

    // create vulkan memory handler
    pMemoryHandler = std::shared_ptr<VulkanMemoryHandler>(new VulkanMemoryHandler(
        vkDevice,
        vkPhysicalDevice,
        vkInstance,
        vkGraphicsQueue,
        vkTransferQueue
    ));

    // create render pipeline
    pRenderPipeline = std::shared_ptr<VulkanRenderPipeline>(new VulkanRenderPipeline(
        vkDevice,
        queueFamilyIndices.graphicsFamily.value(),
        queueFamilyIndices.transferFamily.value()));

    this->pSwapchain->CreateSwapChain();
    pRenderPipeline->Load(
        pSwapchain->GetData(), 
        pSwapchain->GetImageViews());
    
    // create render loop
    pRenderLoop = std::shared_ptr<VulkanRenderLoop>(new VulkanRenderLoop(
        vkDevice, 
        pSwapchain, 
        pRenderPipeline, 
        pMemoryHandler,
        vkGraphicsQueue, 
        vkTransferQueue, 
        vkPresentQueue));
}

void VulkanLoader::Cleanup()
{
    // wait until device is not executing work
    vkDeviceWaitIdle(vkDevice);

    // render loop
    pRenderLoop->CleanUp();
    pRenderLoop.reset();

    // render pipeline
    pRenderPipeline->CleanUp();
    pRenderPipeline.reset();

    // images & swapchain
    pSwapchain->CleanUp();
    pSwapchain.reset();

    // memory
    pMemoryHandler->Cleanup();
    pMemoryHandler.reset();

    // devices
    vkDestroyDevice(vkDevice, nullptr);

    // debug
    vulkanDestroyDebugMessenger();

    // surface
    vkDestroySurfaceKHR(vkInstance, vkSurface, nullptr);

    //instance
    vkDestroyInstance(vkInstance, nullptr);
}

void VulkanLoader::vulkanCreateInstance(const VulkanWindow& vulkanWindow)
{
    // create app info
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "ArcticGame";
    appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.pEngineName = "ArcticEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

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
    auto extensions = vulkanGetRequiredExtensions(vulkanWindow);
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

std::vector<const char*> VulkanLoader::vulkanGetRequiredExtensions(const VulkanWindow & vulkanWindow)
{
    // create empty extensions
    std::vector<const char*> extensions;

    // get glfw extensions
    auto windowExtensions = vulkanWindow.GetExtensions();
    size_t windowExtensionCount = windowExtensions.size();
    //const char** glfwExtensions = windowExtensions.second;

    for(int i=0; i<windowExtensionCount; ++i)
    {
        const char* extension = windowExtensions[i];
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

#pragma region vk_devices

void VulkanLoader::vulkanLoadPhysicalDevice(const VkInstance& instance, const VkSurfaceKHR& surface)
{
    // get available physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if(deviceCount == 0)
    {
        std::cout << "error: vulkan: did not find physical device!";
        return;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // find suitable device
    // todo: idea: could add score implementation (more features = better score), select device with highest score
    vkPhysicalDevice = VK_NULL_HANDLE;

    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    QueueFamilyIndices queueFamilyIndices;

    for(auto &physicalDevice : devices)
    {
        // get data of device
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
        vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
        queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

        // break loop when found suitable device
        if(isVkDeviceSuitable(physicalDevice, surface, deviceProperties, deviceFeatures, queueFamilyIndices))
        {
            vkPhysicalDevice = physicalDevice;
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

void VulkanLoader::vulkanCreateLogicalDevice(const VkPhysicalDevice& physicalDevice, QueueFamilyIndices indices)
{
    // create device queue infos
    // >> create set of queue families (re-use queue families instead of creating duplicates)
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value(), indices.transferFamily.value() };

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
    VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &vkDevice);
    if(result != VK_SUCCESS)
    {
        std::cout << "error: vulkan: failed to create logical device!";
        return;
    }

    // get graphics queue
    vkGetDeviceQueue(vkDevice, indices.graphicsFamily.value(), 0, &vkGraphicsQueue);

    // get transfer queue
    vkGetDeviceQueue(vkDevice, indices.transferFamily.value(), 0, &vkTransferQueue);

    // get present queue
    vkGetDeviceQueue(vkDevice, indices.presentFamily.value(), 0, &vkPresentQueue);
}

bool VulkanLoader::isVkDeviceSuitable(
        const VkPhysicalDevice& physicalDevice,
        const VkSurfaceKHR& surface,
        VkPhysicalDeviceProperties deviceProperties,
        VkPhysicalDeviceFeatures deviceFeatures,
        QueueFamilyIndices queueFamilyIndices) const
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
    bool foundDeviceExtensions = findRequiredDeviceExtensions(physicalDevice);
    if(!foundDeviceExtensions)
        return false;

    // check if swap chain is valid
    // >> see device & surface
    SwapChainDeviceSupport swapChainSupport = this->pSwapchain->QuerySwapChainSupport(physicalDevice, surface);
    bool isSwapChainValid = !swapChainSupport.surfaceFormats.empty() && !swapChainSupport.presentModes.empty();
    if(!isSwapChainValid)
        return false;

    return true;
}

VulkanLoader::QueueFamilyIndices VulkanLoader::findQueueFamilies(const VkPhysicalDevice & device, const VkSurfaceKHR & surface)
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
        if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && 
           !queueFamilyIndices.graphicsFamily.has_value())
            queueFamilyIndices.graphicsFamily = familyIndex;

        // set transfer family
        if((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && 
          !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && 
            !queueFamilyIndices.transferFamily.has_value())
            queueFamilyIndices.transferFamily = familyIndex;

        // set present family
        VkBool32 isPresentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, familyIndex, surface, &isPresentSupport);
        if(isPresentSupport)
            queueFamilyIndices.presentFamily = familyIndex;

        ++familyIndex;
    }
    return queueFamilyIndices;
}

bool VulkanLoader::findRequiredDeviceExtensions(const VkPhysicalDevice & device) const
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

#pragma endregion vk_devices

#pragma region vk_pipeline


#pragma endregion vk_pipeline

#pragma region vk_validation

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
        VkDebugUtilsMessengerEXT debugUtilsMessenger,
        const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr){
        func(instance, debugUtilsMessenger, pAllocator);
    }
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

#pragma endregion vk_validation
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

class HelloTriangleApplication
{
public:
    void run()
    {
        initWindow();
        vulkanInitialization();
        mainLoop();
        cleanup();
    }

private:

    // glfw
    const uint32_t WINDOW_WIDTH = 1280;
    const uint32_t WINDOW_HEIGHT = 720;
    GLFWwindow* window = nullptr;

    // vulkan
    VkInstance vkInstance = nullptr;

    const bool enableValidationLayers = false;

    VkDebugUtilsMessengerEXT debugMessenger;
    const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation" // all the useful standard validation is bundled into a layer included in the SDK
    };

    void initWindow()
    {
        // init glfw
        glfwInit();

        // set hints
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        // create window
        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);

    }

    void vulkanInitialization()
    {
        // check validation layers
        if(enableValidationLayers && !vulkanFoundValidationLayers())
        {
            std::cout << "error: vulkan: validation layers requested, but not available!";
            return;
        }

        // create instance
        vulkanCreateInstance();
        vulkanSetupDebugMessenger();
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }

    void cleanup()
    {
        // vulkan
        vulkanDestroyDebugMessenger();
        vkDestroyInstance(vkInstance, nullptr);

        // glfw
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void vulkanCreateInstance()
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

    std::vector<const char*> vulkanGetRequiredExtensions()
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

    void vulkanSetupDebugMessenger()
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

    void vulkanDestroyDebugMessenger()
    {
        if (enableValidationLayers)
            vulkanDestroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, nullptr);
    }

    bool vulkanFoundValidationLayers()
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

    VkResult vulkanCreateDebugUtilsMessengerEXT(
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

    void vulkanDestroyDebugUtilsMessengerEXT(
            VkInstance instance,
            VkDebugUtilsMessengerEXT debugMessenger,
            const VkAllocationCallbacks* pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
            func(instance, debugMessenger, pAllocator);
    }

    void vulkanPopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
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

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData)
            {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }
};

int main()
{
    HelloTriangleApplication app;
    app.run();
    return 0;
}
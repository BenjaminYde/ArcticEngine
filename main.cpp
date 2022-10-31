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
        initVulkan();
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

    void initVulkan()
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
        createInfo.enabledLayerCount = 0;

        // search for glfw extension
        // >> append data to create info
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        // .. debug extensions
        std::vector<VkExtensionProperties> extensions(glfwExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &glfwExtensionCount, extensions.data());

        std::cout << "available extensions: " << std::endl;
        for( const auto& extension : extensions)
        {
            std::cout << "\t" << extension.extensionName << std::endl;
        }

        // create vk instance
        VkResult result = vkCreateInstance(&createInfo, nullptr, &vkInstance);
        if( result != VK_SUCCESS)
        {
            std::cout << "failed to create instance!";
            return;
        }
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
        vkDestroyInstance(vkInstance, nullptr);

        // glfw
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main()
{
    HelloTriangleApplication app;
    app.run();
    return 0;
}
#include <GLFW/glfw3.h>
#include "vulkan_loader.h"
#include "engine/arctic_engine.h"

void ArcticEngine::run()
{
    // loop while no close window
    auto window = vulkanLoader->GetWindow();
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }
}

void ArcticEngine::initialize()
{
    // load vulkan
    vulkanLoader = new VulkanLoader();
    vulkanLoader->Load();
}

void ArcticEngine::cleanup()
{
    // cleanup vulkan
    vulkanLoader->Cleanup();
    delete vulkanLoader;
}

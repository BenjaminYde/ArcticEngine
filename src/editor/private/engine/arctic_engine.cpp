#include <GLFW/glfw3.h>
#include "vulkan_loader.h"
#include "engine/arctic_engine.h"

void ArcticEngine::run()
{
    // load
    vulkanLoader = new VulkanLoader();
    vulkanLoader->Load();

    // loop
    mainLoop();

    // cleanup
    vulkanLoader->Cleanup();
    delete vulkanLoader;
}

void ArcticEngine::mainLoop()
{
    auto window = vulkanLoader->GetWindow();
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }
}
#ifndef ARCTIC_ARCTIC_ENGINE_H
#define ARCTIC_ARCTIC_ENGINE_H

class VulkanLoader;

class ArcticEngine
{
public:
    void run();

private:
    VulkanLoader* vulkanLoader;

    void mainLoop();
};

#endif //ARCTIC_ARCTIC_ENGINE_H

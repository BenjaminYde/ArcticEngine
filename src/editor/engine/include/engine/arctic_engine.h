#ifndef ARCTIC_ARCTIC_ENGINE_H
#define ARCTIC_ARCTIC_ENGINE_H

class VulkanLoader;

class ArcticEngine
{
public:
    void initialize();
    void run();
    void cleanup();
private:
    VulkanLoader* vulkanLoader;
};

#endif //ARCTIC_ARCTIC_ENGINE_H

#include <iostream>
#include <vulkan/vulkan.h>
#include <format>

int main()
{
    VkExtent2D windowExtend{ 1280 , 720 };

    std::cout << "Hello World!" << std::endl;
    std::cout << std::format("Window extend w: {} h: {}", windowExtend.width, windowExtend.height) << std::endl;
    return 0;
}
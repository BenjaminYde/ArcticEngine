#include <iostream>
#include <vulkan/vulkan.h>
#include <format>
#include "glm/vec3.hpp"
#include "glm/geometric.hpp"

int main()
{
    VkExtent2D windowExtend{ 1280 , 720 };
    glm::vec3 vector = glm::vec3(0.5f, 0.5f, 0.5f);

    std::cout << "Hello World!" << std::endl;
    std::cout << std::format("vk window extend w: {} h: {}", windowExtend.width, windowExtend.height) << std::endl;
    std::cout << std::format("glm vector length: {}", glm::length(vector)) << std::endl;
    return 0;
}
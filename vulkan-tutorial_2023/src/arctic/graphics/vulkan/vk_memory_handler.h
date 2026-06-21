#pragma once

#include <vulkan/vulkan_core.h>
#include "vk_mem_alloc.h"

class VulkanMemoryHandler
{
public:
    VulkanMemoryHandler(VkDevice& vkDevice, VkPhysicalDevice& vkPhysicalDevice, VkInstance& vkInstance, VkQueue& vkGraphicsQueue, VkQueue& vkTransferQueue);
    void Cleanup();
    
    bool CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    bool CopyBufferToBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool& commandPool);
    bool FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, uint32_t& memoryTypeIndex);
    bool CopyDataToBuffer(void* pDataToCopy, VkDeviceSize bufferSize, VkDeviceMemory memory);

    bool CreateBufferVMA(VkDeviceSize bufferSize, VkBufferUsageFlags usage, VmaAllocationCreateFlags vmaFlags, VkBuffer *pBuffer, VmaAllocation *pBufferAllocation);
    bool CopyDataToBufferVMA(void* pDataToCopy, VkDeviceSize bufferSize, VmaAllocation &bufferAllocation);

    VmaAllocator& GetAllocator();
private:
    VkDevice vkDevice;
    VkPhysicalDevice vkPhysicalDevice;
    VkQueue vkGraphicsQueue;
    VkQueue vkTransferQueue;
    VmaAllocator vmaAllocator;
};
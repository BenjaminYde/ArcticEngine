#include "vk_memory_handler.h"
#include <iostream>
#include <vector>
#include <cstring>
#include "vk_mem_alloc.h"

VulkanMemoryHandler::VulkanMemoryHandler(
    VkDevice& vkDevice,
    VkPhysicalDevice& vkPhysicalDevice,
    VkInstance& vkInstance,
    VkQueue& vkGraphicsQueue,
    VkQueue& vkTransferQueue)
:
vkDevice(vkDevice),
vkPhysicalDevice(vkPhysicalDevice),
vkGraphicsQueue(vkGraphicsQueue),
vkTransferQueue(vkTransferQueue)
{
    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    allocatorCreateInfo.physicalDevice = vkPhysicalDevice;
    allocatorCreateInfo.device = vkDevice;
    allocatorCreateInfo.instance = vkInstance;
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator);
}

void VulkanMemoryHandler::Cleanup()
{
    vmaDestroyAllocator(this->vmaAllocator);
}

/// @brief creates a buffer and device memory for the buffer
/// @param size defines the size of the buffer
/// @param usage defines how the buffer can be used using bit flags (src / dst / vertex / ...)
/// @param properties defines the memory properties of the buffer
/// @param buffer reference to the actual buffer
/// @param bufferMemory reference to the actual memory the buffer will be using
/// @return
bool VulkanMemoryHandler::CreateBuffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& bufferMemory)
{
    // create buffer info
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // the following may be needed when using CONCURRENT sharing mode
    //bufferInfo.queueFamilyIndexCount = 2;
    //std::vector<uint32_t> allowedQueueIndices { 0, 1};
    //bufferInfo.pQueueFamilyIndices = allowedQueueIndices.data();

    // create buffer
    VkResult result = vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &buffer);
    if (result != VK_SUCCESS)
    {
        std::cout << "error: vulkan: failed to create buffer!";
        return false;
    }

    // create memory allocation info (for the buffer)
    VkMemoryRequirements vbMemoryRequirements;
    vkGetBufferMemoryRequirements(vkDevice, buffer, &vbMemoryRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = vbMemoryRequirements.size;

    uint32_t memoryTypeIndex;
    if(!FindMemoryType(vbMemoryRequirements.memoryTypeBits, properties, memoryTypeIndex))
    {
        std::cout << "error: vulkan: failed to allocate memory!";
        return false;
    }
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    // create buffer memory
    result = vkAllocateMemory(this->vkDevice, &allocInfo, nullptr, &bufferMemory);
    if (result != VK_SUCCESS)
    {
        std::cout << "error: vulkan: failed to allocate memory!";
        return false;
    }

    // bind the memory to the buffer
    vkBindBufferMemory(this->vkDevice, buffer, bufferMemory, 0);
    return true;
}

/// @brief copies buffer data from source to dest
/// @param srcBuffer
/// @param dstBuffer
/// @param size
bool VulkanMemoryHandler::CopyBufferToBuffer(
    VkBuffer srcBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize size,
    VkCommandPool& commandPool)
{
    // create info: command buffer allocation
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    // create command buffer
    VkCommandBuffer commandBuffer;
    VkResult result = vkAllocateCommandBuffers(this->vkDevice, &allocInfo, &commandBuffer);
    if (result != VK_SUCCESS)
    {
        std::cout << "error: vulkan: failed to create command buffer!";
        return false;
    }

    // command buffer: start executing commands
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // command: copy the buffers src >> dst
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    // command buffer: end executing commands
    vkEndCommandBuffer(commandBuffer);

    // execute the command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(this->vkTransferQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(this->vkTransferQueue);

    // cleanup
    vkFreeCommandBuffers(this->vkDevice, commandPool, 1, &commandBuffer);
    return true;
}

/// @brief
/// @param typeFilter used to specify the bit field of memory types that are suitable
/// @param properties
/// @param memoryType
/// @return
bool VulkanMemoryHandler::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, uint32_t& memoryTypeIndex)
{
    // get memory properties of physical device
    // >> has memory types: distinct memory resources like dedicated VRAM
    // >> has memory heaps: the different types of memory exist within these heaps
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memProperties);

    // loop over all properties
    // >> try find matching memorty index
    for (uint32_t i=0; i < memProperties.memoryTypeCount; i++)
    {
        // typeFilter: we can find the index of a suitable memory by checking if the corresponding bit is set to 1
        //
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            memoryTypeIndex = i;
            return true;
        }
    }
    return false;
}

bool VulkanMemoryHandler::CopyDataToBuffer(void *pDataToCopy, VkDeviceSize bufferSize, VkDeviceMemory memory)
{
    void* data;
    if(vkMapMemory(this->vkDevice, memory, 0, bufferSize, 0, &data) != VK_SUCCESS)
        return false;
    memcpy(data, pDataToCopy, bufferSize);
    vkUnmapMemory(this->vkDevice, memory);
    return true;
}

bool VulkanMemoryHandler::CreateBufferVMA(VkDeviceSize bufferSize, VkBufferUsageFlags usage, VmaAllocationCreateFlags vmaFlags, VkBuffer *pBuffer, VmaAllocation *pBufferAllocation)
{
    // create buffer info
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // create buffer vma
    VmaAllocationCreateInfo allocCreateInfo{};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags = vmaFlags;

    // create buffer vma
    if (vmaCreateBuffer(this->vmaAllocator, &bufferInfo, &allocCreateInfo, pBuffer, pBufferAllocation, nullptr) != VK_SUCCESS)
        return false;
    return true;
}

bool VulkanMemoryHandler::CopyDataToBufferVMA(void* pDataToCopy, VkDeviceSize bufferSize, VmaAllocation &bufferAllocation)
{
    void* data;
    if(vmaMapMemory(this->vmaAllocator, bufferAllocation, &data) != VK_SUCCESS)
        return false;
    memcpy(data, pDataToCopy, bufferSize);
    vmaUnmapMemory(this->vmaAllocator, bufferAllocation);
    return true;
}

VmaAllocator &VulkanMemoryHandler::GetAllocator()
{
    return this->vmaAllocator;
}

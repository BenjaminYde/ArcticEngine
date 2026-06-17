#pragma once

#include <vector>
#include <memory>
#include <vulkan/vulkan_core.h>
#include "vk_mem_alloc.h"

class VulkanSwapChain;
class VulkanRenderPipeline;
class VulkanMemoryHandler;
class Vertex;

class VulkanRenderLoop
{
public:
    VulkanRenderLoop(
        VkDevice vkDevice,
        std::shared_ptr<VulkanSwapChain> swapChain, 
        std::shared_ptr<VulkanRenderPipeline> renderPipeline,
        std::shared_ptr<VulkanMemoryHandler> vkMemoryHandler, 
        VkQueue GraphicsQueue,
        VkQueue vkTransferQueue,
        VkQueue vkPresentQueue);
    
    void Render();
    void CleanUp();

    bool IsSwapChainDirty() const;

private:

    // devices
    VkDevice vkDevice = VK_NULL_HANDLE;
    std::shared_ptr<VulkanSwapChain> pSwapchain;
    std::shared_ptr<VulkanRenderPipeline> pRenderPipeline;

    // commands
    VkCommandPool vkCommandPoolGraphics;
    VkCommandPool vkCommandPoolTransfer;

    VkQueue vkGraphicsQueue;
    VkQueue vkTransferQueue;
    VkQueue vkPresentQueue;

    // syncing
    const int MAX_FRAMES_IN_FLIGHT = 3;

    uint16_t currentFrameIndex = 0;

    struct Frame
    {   
        // commands
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

        // syncing
        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
        VkFence isDoneRenderingFence = VK_NULL_HANDLE;

        // memory
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

        VkBuffer uniformBuffer = VK_NULL_HANDLE;
        VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;
        void* uniformBufferMapped = nullptr;
    };

    std::vector<std::unique_ptr<Frame>> frames;

    bool isSwapChainDirty;

    // memory
    std::shared_ptr<VulkanMemoryHandler> vkMemoryHandler;

    VkDescriptorPool vkDescriptorPool;

    // .. mesh
    VkBuffer vertexBuffer;
    VmaAllocation vertexBufferAllocation;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    // .. image
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;

    // commands
    void createCommandPool(uint32_t graphicsFamilyIndex, uint32_t transferFamilyIndex);
    void createCommandBuffers();
    
    void recordCommandBuffer(const Frame& frame, uint32_t imageIndex);
    void updateUniformBuffer(const Frame& frame);

    // memory
    bool createVertexBuffer(std::vector<Vertex> vertices);
    bool createIndexBuffer(std::vector<uint32_t> indices);

    bool createUniformBuffers();
    bool createDescriptorPool();
    bool createDescriptorSets();

    // syncing
    void createSyncObjects();

    // images
    void createTextureImage();
};
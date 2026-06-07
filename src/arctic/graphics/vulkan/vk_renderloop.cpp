#include "vk_renderloop.h"

#include "arctic/core/utilities/application.h"
#include "arctic/graphics/rhi/uniform_buffer_object.h"
#include "arctic/graphics/rhi/vertex.h"
#include "vk_memory_handler.h"
#include "vk_renderpipeline.h"
#include "vk_swapchain.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <cstring>
#include <format>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

VulkanRenderLoop::VulkanRenderLoop(VkDevice vkDevice, std::shared_ptr<VulkanSwapChain> swapChain,
                                   std::shared_ptr<VulkanRenderPipeline> renderPipeline,
                                   std::shared_ptr<VulkanMemoryHandler>  vkMemoryHandler,
                                   VkQueue graphicsQueue, VkQueue transferQueue,
                                   VkQueue presentQueue)
    : vkDevice(vkDevice), pSwapchain(swapChain), pRenderPipeline(renderPipeline),
      vkMemoryHandler(vkMemoryHandler), vkGraphicsQueue(graphicsQueue),
      vkTransferQueue(transferQueue), vkPresentQueue(presentQueue) {
    // define frames to use
    frames.resize(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        this->frames[i] = std::unique_ptr<Frame>(new Frame());
    }

    // create command pool and buffer
    createCommandPool(renderPipeline->GetGraphicsFamilyIndex(),
                      renderPipeline->GetTransferFamilyIndex());
    createCommandBuffers();

    // syncing
    createSyncObjects();

    // create vertex buffer
    const std::vector<Vertex> vertices = {{{-0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
                                          {{0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
                                          {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
                                          {{-0.5f, 0.5f}, {0.0f, 1.0f, 0.25f}}};

    const std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};

    if (!createVertexBuffer(vertices)) {
        std::cout << "error: vulkan: failed to create vertex buffer!";
        return;
    }

    if (!createIndexBuffer(indices)) {
        std::cout << "error: vulkan: failed to create index buffer!";
        return;
    }

    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();

    // image loading
    createTextureImage();
}

void VulkanRenderLoop::CleanUp() {
    // wait until device is not executing work
    vkDeviceWaitIdle(vkDevice);

    // syncing
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        auto& frame = this->frames[i];

        vkDestroySemaphore(vkDevice, frame->imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(vkDevice, frame->renderFinishedSemaphore, nullptr);
        vkDestroyFence(vkDevice, frame->isDoneRenderingFence, nullptr);
    }

    // command pool & buffer
    vkDestroyCommandPool(vkDevice, vkCommandPoolGraphics, nullptr);
    vkDestroyCommandPool(vkDevice, vkCommandPoolTransfer, nullptr);

    // buffers
    vkDestroyBuffer(vkDevice, vertexBuffer, nullptr);
    this->vertexBuffer = VK_NULL_HANDLE;
    // vkFreeMemory(vkDevice, vertexBufferMemory, nullptr);

    vmaDestroyBuffer(vkMemoryHandler->GetAllocator(), this->vertexBuffer,
                     this->vertexBufferAllocation);

    vkDestroyBuffer(vkDevice, indexBuffer, nullptr);
    vkFreeMemory(vkDevice, indexBufferMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        auto& frame = this->frames[i];

        vkDestroyBuffer(vkDevice, frame->uniformBuffer, nullptr);
        vkFreeMemory(vkDevice, frame->uniformBufferMemory, nullptr);

        // destroy frame
        frame.reset();
    }

    vkDestroyDescriptorPool(vkDevice, vkDescriptorPool, nullptr);
}

bool VulkanRenderLoop::IsSwapChainDirty() const {
    return this->isSwapChainDirty;
}

void VulkanRenderLoop::Render() {
    // wait until previous frame is finished
    //// todo: implement multiple frames in flight. this avoids idle time (cpu waiting for gpu &
    /// vice versa) / https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Frames_in_flight
    auto& frame = this->frames[currentFrameIndex];
    vkWaitForFences(vkDevice, 1, &frame->isDoneRenderingFence, VK_TRUE, UINT64_MAX);

    // acquire next image from swap chain

    // try acquire next image
    uint32_t availableImageIndex = 0;
    VkResult resultAcquireNextImage =
        vkAcquireNextImageKHR(vkDevice, pSwapchain->GetSwapChain(), UINT64_MAX,
                              frame->imageAvailableSemaphore, VK_NULL_HANDLE, &availableImageIndex);

    // check if swapchain still up-to-date
    // >> recreate when not (due to window resizing, ...)
    if (resultAcquireNextImage == VK_ERROR_OUT_OF_DATE_KHR) {
        // wait until idle
        vkDeviceWaitIdle(vkDevice);

        // mark dirty
        this->isSwapChainDirty = true;
        return;
    } else {
        this->isSwapChainDirty = false;
    }

    // reset fence state to zero
    // only reset the fence if we are submitting work
    vkResetFences(vkDevice, 1, &frame->isDoneRenderingFence);

    // record command buffer
    vkResetCommandBuffer(frame->commandBuffer, 0);
    recordCommandBuffer(*frame, availableImageIndex);

    // create info: command buffer submit
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    //> specify semaphores to wait on before execution
    VkSemaphore          waitSemaphores[] = {frame->imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[]     = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount         = 1;
    submitInfo.pWaitSemaphores            = waitSemaphores;
    submitInfo.pWaitDstStageMask          = waitStages;

    //> specify command buffer
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &frame->commandBuffer;

    //> signal semaphores on finish execution
    VkSemaphore signalSemaphores[]  = {frame->renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    // submit command buffer to graphics queue
    VkResult resultQueueSubmit =
        vkQueueSubmit(vkGraphicsQueue, 1, &submitInfo, frame->isDoneRenderingFence);
    if (resultQueueSubmit != VK_SUCCESS) {
        std::cout << "error: vulkan: failed to submit command buffer to graphics queue!";
        return;
    }

    // create info: present khr
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;

    VkSwapchainKHR swapChains[] = {pSwapchain->GetSwapChain()};
    presentInfo.swapchainCount  = 1;
    presentInfo.pSwapchains     = swapChains;
    presentInfo.pImageIndices   = &availableImageIndex;
    presentInfo.pResults        = nullptr; // Optional

    // queue present khr
    vkQueuePresentKHR(vkPresentQueue, &presentInfo);

    // increase current image frame
    currentFrameIndex = (currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderLoop::createCommandPool(uint32_t graphicsFamilyIndex,
                                         uint32_t transferFamilyIndex) {
    // create info: command pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags =
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // we record a command buffer every frame,
                                                         // so we want to be able to reset and
                                                         // re-record
    poolInfo.queueFamilyIndex = graphicsFamilyIndex;

    // create command pool
    VkResult result = vkCreateCommandPool(vkDevice, &poolInfo, nullptr, &vkCommandPoolGraphics);
    if (result != VK_SUCCESS) {
        std::cout << "error: vulkan: failed to create command pool!";
        return;
    }

    // create info: command pool
    VkCommandPoolCreateInfo poolInfo2{};
    poolInfo2.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo2.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo2.queueFamilyIndex = transferFamilyIndex;

    // create command pool
    result = vkCreateCommandPool(vkDevice, &poolInfo2, nullptr, &vkCommandPoolTransfer);
    if (result != VK_SUCCESS) {
        std::cout << "error: vulkan: failed to create command pool!";
        return;
    }
}

void VulkanRenderLoop::createCommandBuffers() {
    // create all buffers
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // get frame
        auto& frame = this->frames[i];

        // create info: command buffer allocation
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = vkCommandPoolGraphics;
        allocInfo.level =
            VK_COMMAND_BUFFER_LEVEL_PRIMARY; // can be submitted to a queue for execution, but
                                             // cannot be called from other command buffers
        allocInfo.commandBufferCount = 1;

        // create command buffers
        // this->vkCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        VkResult result = vkAllocateCommandBuffers(vkDevice, &allocInfo, &frame->commandBuffer);
        if (result != VK_SUCCESS) {
            std::cout << "error: vulkan: failed to create command buffer!";
            return;
        }
    }
}

bool VulkanRenderLoop::createVertexBuffer(std::vector<Vertex> vertices) {
    VkDeviceSize             bufferSize = sizeof(Vertex) * vertices.size();
    VmaAllocationCreateFlags vmaFlags =
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    // staging buffer:
    // >> with: the data is first copied to a CPU-friendly staging buffer and then transferred to
    // the final GPU buffer, which is optimized for fast GPU access. This method is more efficient
    // for GPU performance because the final buffer resides in device-local memory, which is faster
    // for rendering.
    // >> without: the data is copied directly into a buffer that can be accessed by both the CPU
    // and GPU, but this memory type is not as fast for the GPU. While simpler, this method can
    // reduce rendering performance because the buffer is not optimized for GPU access.
    bool useStagingBuffer = true;
    if (useStagingBuffer) {
        // create staging buffer (CPU-accessible)
        // >> buffer will serve as the source for data transfer operations to the GPU
        VkBuffer           vertexBufferStaging;
        VmaAllocation      bufferAllocationStaging;
        VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        if (!vkMemoryHandler->CreateBufferVMA(bufferSize, bufferUsage, vmaFlags,
                                              &vertexBufferStaging, &bufferAllocationStaging))
            return false;

        // copy vertex data to staging buffer (CPU memory)
        if (!vkMemoryHandler->CopyDataToBufferVMA(vertices.data(), bufferSize,
                                                  bufferAllocationStaging))
            return false;

        // create final vertex buffer (GPU-accessible)
        // >> buffer will be used as the destination for the data transfer and for rendering
        // >> allocated in device-local memory, not directly accessible by the CPU which generally
        // means that we're not able to use vkMapMemory
        bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        vmaFlags    = 0;
        if (!vkMemoryHandler->CreateBufferVMA(bufferSize, bufferUsage, vmaFlags,
                                              &this->vertexBuffer, &this->vertexBufferAllocation))
            return false;

        // transfer vertex data from staging buffer to final vertex buffer (GPU memory)
        if (!vkMemoryHandler->CopyBufferToBuffer(vertexBufferStaging, this->vertexBuffer,
                                                 bufferSize, this->vkCommandPoolTransfer))
            return false;

        // clean up staging buffer (no longer needed after data transfer)
        vmaDestroyBuffer(vkMemoryHandler->GetAllocator(), vertexBufferStaging,
                         bufferAllocationStaging);
    } else {
        // create vertex buffer (GPU interaction)
        // >> directly maps and copies vertex data into the GPU-accessible buffer
        VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        if (!vkMemoryHandler->CreateBufferVMA(bufferSize, bufferUsage, vmaFlags, &vertexBuffer,
                                              &vertexBufferAllocation))
            return false;

        // copy vertex data directly to the buffer (CPU memory)
        if (!vkMemoryHandler->CopyDataToBufferVMA(vertices.data(), bufferSize,
                                                  this->vertexBufferAllocation))
            return false;
    }
    return true;
}

bool VulkanRenderLoop::createIndexBuffer(std::vector<uint32_t> indices) {
    VkDeviceSize bufferSize = sizeof(Vertex) * indices.size();

    bool useStagingBuffer = true;
    if (useStagingBuffer) {

        VkBuffer       indexBufferStaging;
        VkDeviceMemory indexBufferMemoryStaging;

        // create staging buffer (CPU-accessible)
        // >> buffer will serve as the source for data transfer to the GPU
        {
            VkBufferUsageFlags    usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            VkMemoryPropertyFlags memoryProperties =
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            if (!vkMemoryHandler->CreateBuffer(bufferSize, usage, memoryProperties,
                                               indexBufferStaging, indexBufferMemoryStaging))
                return false;

            // copy index data to staging buffer (CPU memory)
            if (!vkMemoryHandler->CopyDataToBuffer(indices.data(), bufferSize,
                                                   indexBufferMemoryStaging))
                return false;
        }

        // create final index buffer (GPU-accessible)
        // >> buffer will be the destination for the data transfer and used for rendering
        // >> allocated in device-local memory, meaning it's optimized for GPU but not
        // CPU-accessible
        {
            VkBufferUsageFlags usage =
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            if (!vkMemoryHandler->CreateBuffer(bufferSize, usage, memoryProperties,
                                               this->indexBuffer, this->indexBufferMemory))
                return false;
        }

        // transfer index data from staging buffer to final index buffer (GPU memory)
        vkMemoryHandler->CopyBufferToBuffer(indexBufferStaging, this->indexBuffer, bufferSize,
                                            this->vkCommandPoolTransfer);

        // cleanup staging buffer (no longer needed after data transfer)
        vkDestroyBuffer(this->vkDevice, indexBufferStaging, nullptr);
        vkFreeMemory(this->vkDevice, indexBufferMemoryStaging, nullptr);
    } else {
        // create index buffer (CPU-accessible)
        // >> this buffer will be directly used by both CPU and GPU
        VkDeviceSize          bufferSize = sizeof(Vertex) * indices.size();
        VkBufferUsageFlags    usage      = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        VkMemoryPropertyFlags memoryPropeties =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        if (!vkMemoryHandler->CreateBuffer(bufferSize, usage, memoryPropeties, this->indexBuffer,
                                           this->indexBufferMemory))
            return false;

        // copy index data directly to buffer (CPU memory)
        if (!vkMemoryHandler->CopyDataToBuffer(indices.data(), bufferSize, this->indexBufferMemory))
            return false;
    }
    return true;
}

/// @brief Create multiple uniform buffers.
/// @brief We're going to copy new data to the uniform buffer every frame, it doesn't really make
/// any sense to have a staging buffer.
/// @brief We should have multiple buffers, because multiple frames may be in flight at the same
/// time and we don't want to update
/// @brief the buffer in preparation of the next frame while a previous one is still reading from
/// it. Thus, we need to have as many
/// @brief uniform buffers as we have frames in flight, and write to a uniform buffer that is not
/// currently being read by the GPU
/// @return true when creation was successful
bool VulkanRenderLoop::createUniformBuffers() {
    // resize buffers
    // uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    // uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    // uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    // create all buffers
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // get frame
        auto& frame = this->frames[i];

        // define buffer
        VkDeviceSize          bufferSize = sizeof(UniformBufferObject);
        VkBufferUsageFlags    usage      = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        VkMemoryPropertyFlags memoryFlags =
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        // create buffer
        if (!vkMemoryHandler->CreateBuffer(bufferSize, usage, memoryFlags, frame->uniformBuffer,
                                           frame->uniformBufferMemory))
            return false;

        // map memory to buffer
        vkMapMemory(this->vkDevice, frame->uniformBufferMemory, 0, bufferSize, 0,
                    &frame->uniformBufferMapped);
    }
    return true;
}

void VulkanRenderLoop::updateUniformBuffer(const Frame& frame) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto  currentTime = std::chrono::high_resolution_clock::now();
    float time =
        std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model =
        glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                           glm::vec3(0.0f, 0.0f, 1.0f));

    auto swapchainData   = pSwapchain->GetData();
    auto swapchainExtent = swapchainData.extent;

    ubo.proj = glm::perspective(glm::radians(45.0f),
                                swapchainExtent.width / (float)swapchainExtent.height, 0.1f, 10.0f);

    ubo.proj[1][1] *= -1;

    memcpy(frame.uniformBufferMapped, &ubo, sizeof(ubo));
}

bool VulkanRenderLoop::createDescriptorPool() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = &poolSize;
    poolInfo.maxSets       = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkResult result =
        vkCreateDescriptorPool(this->vkDevice, &poolInfo, nullptr, &this->vkDescriptorPool);
    if (result != VK_SUCCESS) {
        std::cout << "error: vulkan: failed to create descriptor pool!";
        return false;
    }

    return true;
}

bool VulkanRenderLoop::createDescriptorSets() {
    // allocate
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // get frame
        auto& frame = this->frames[i];

        // create
        auto descriptorSetLayout = this->pRenderPipeline->GetDescriptorSetLayout();
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = this->vkDescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts        = &descriptorSetLayout;

        // this->vkDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        VkResult result =
            vkAllocateDescriptorSets(this->vkDevice, &allocInfo, &frame->descriptorSet);
        if (result != VK_SUCCESS) {
            std::cout << "error: vulkan: failed to create descriptor sets!";
            return false;
        }

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = frame->uniformBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range  = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet           = frame->descriptorSet;
        descriptorWrite.dstBinding       = 0;
        descriptorWrite.dstArrayElement  = 0;
        descriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount  = 1;
        descriptorWrite.pBufferInfo      = &bufferInfo;
        descriptorWrite.pImageInfo       = nullptr; // Optional
        descriptorWrite.pTexelBufferView = nullptr; // Optional

        vkUpdateDescriptorSets(this->vkDevice, 1, &descriptorWrite, 0, nullptr);
    }
    return true;
}

void VulkanRenderLoop::recordCommandBuffer(const Frame& frame, uint32_t imageIndex) {
    // get command buffer
    VkCommandBuffer commandBuffer = frame.commandBuffer;

    // command buffer: begin
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags            = 0;       // optional
    beginInfo.pInheritanceInfo = nullptr; // optional

    VkResult resultBeginCommandBuffer = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (resultBeginCommandBuffer != VK_SUCCESS) {
        std::cout << "error: vulkan: failed to begin command buffer!";
        return;
    }

    // get swapchain data
    SwapChainData swapChainData = pSwapchain->GetData();

    // command buffer: begin render pass
    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass  = pRenderPipeline->GetRenderPass();
    renderPassBeginInfo.framebuffer = pRenderPipeline->GetFrameBuffer(imageIndex);

    renderPassBeginInfo.renderArea.offset = VkOffset2D{0, 0};
    renderPassBeginInfo.renderArea.extent = swapChainData.extent;

    VkClearValue clearColor             = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues    = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // command buffer: bind to pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pRenderPipeline->GetPipeline());

    // command buffer: set viewport
    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (float)swapChainData.extent.width;
    viewport.height   = (float)swapChainData.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    // command buffer: set scissor
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainData.extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // command buffer: bind vertex buffer
    VkBuffer     vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[]       = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    // command buffer: bind index buffer
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // command buffer: bind descriptor sets
    updateUniformBuffer(frame);
    auto pipelineLayout = pRenderPipeline->GetPipelineLayout();
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                            &frame.descriptorSet, 0, nullptr);

    // command buffer: draw
    uint32_t vertexCount = 3;
    // vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);

    uint32_t indexCount = 6;
    vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);

    // command buffer: end render pass
    vkCmdEndRenderPass(commandBuffer);

    // command buffer: end
    VkResult resultEndCommandBuffer = vkEndCommandBuffer(commandBuffer);
    if (resultEndCommandBuffer != VK_SUCCESS) {
        std::cout << "error: vulkan: failed to end command buffer!";
        return;
    }
}

void VulkanRenderLoop::createSyncObjects() {
    // create info
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // is signaled on start

    // resize memory
    // imageAvailableSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
    // renderFinishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
    // isDoneRenderingFence.resize(MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        // get frame
        auto& frame = this->frames[i];

        // create objects
        if (vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &frame->imageAvailableSemaphore) !=
                VK_SUCCESS ||
            vkCreateSemaphore(vkDevice, &semaphoreInfo, nullptr, &frame->renderFinishedSemaphore) !=
                VK_SUCCESS ||
            vkCreateFence(vkDevice, &fenceInfo, nullptr, &frame->isDoneRenderingFence) !=
                VK_SUCCESS) {
            std::cout << "error: vulkan: failed to create sync objects!";
            return;
        }
    }
}

void VulkanRenderLoop::createTextureImage() {
    // read image from path
    int         texWidth, texHeight, texChannels;
    std::string pathTexture = std::format("{}/images/{}", Application::AssetsPath, "texture.jpg");
    // std::cout << pathTexture << std::endl;
    stbi_uc* pixels =
        stbi_load(pathTexture.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        std::cout << "failed to load texture image!";
        return;
    }

    // create vulkan buffers
    VkBufferUsageFlags    usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags memoryProperties =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    if (!vkMemoryHandler->CreateBuffer(imageSize, usage, memoryProperties, this->stagingBuffer,
                                       this->stagingBufferMemory))
        return;

    // copy pixels to memory
    void* data;
    vkMapMemory(this->vkDevice, this->stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, (size_t)imageSize);
    vkUnmapMemory(this->vkDevice, this->stagingBufferMemory);

    // create image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = static_cast<uint32_t>(texWidth);
    imageInfo.extent.height = static_cast<uint32_t>(texHeight);
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags         = 0; // Optional

    if (vkCreateImage(this->vkDevice, &imageInfo, nullptr, &textureImage) != VK_SUCCESS) {
        std::cout << "failed to create texture image!";
        return;
    }

    // bind image to memory
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(this->vkDevice, textureImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    this->vkMemoryHandler->FindMemoryType(memRequirements.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                          allocInfo.memoryTypeIndex);

    if (vkAllocateMemory(this->vkDevice, &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(this->vkDevice, textureImage, textureImageMemory, 0);

    // cleanup
    stbi_image_free(pixels);
}
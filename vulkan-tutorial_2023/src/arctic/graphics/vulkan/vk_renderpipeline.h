#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>
#include "vk_swapchain.h"

class VulkanRenderPipeline
{
public:
  VulkanRenderPipeline(
    const VkDevice& device,
    uint32_t graphicsFamilyIndex,
    uint32_t transferFamilyIndex); 

  void Load(
    const SwapChainData& swapChainData, 
    const std::vector<VkImageView>& swapChainImageViews);

  void CleanUp();

  uint32_t GetGraphicsFamilyIndex();
  uint32_t GetTransferFamilyIndex();
  const VkRenderPass& GetRenderPass();
  const VkPipeline& GetPipeline();
  const VkPipelineLayout& GetPipelineLayout();
  const VkDescriptorSetLayout& GetDescriptorSetLayout();
  const VkFramebuffer& GetFrameBuffer(uint32_t index);

private:
    VkDevice vkDevice = VK_NULL_HANDLE;
    uint32_t graphicsFamilyIndex;
    uint32_t transferFamilyIndex;

    SwapChainData swapChainData;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass vkRenderPass;
    VkDescriptorSetLayout vkDescriptorSetLayout;
    VkPipelineLayout vkPipelineLayout;
    VkPipeline vkPipeline;

    void createRenderPass();
    void createPipeline();
    void createFramebuffers();

    void createDescriptorSetLayout();

    bool createShaderModule(const std::vector<char>& code, VkShaderModule& shaderModule);
};
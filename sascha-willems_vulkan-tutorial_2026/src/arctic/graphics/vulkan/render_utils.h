#pragma once

#include "arctic/graphics/rhi/vertex.h"
#include <array>
#include <vulkan/vulkan_core.h>

class RenderUtils {
  public:
    static VkVertexInputBindingDescription                  GetBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions();
};
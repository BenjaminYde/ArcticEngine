#include "render_utils.h"

/// @brief Creates a description that tells Vulkan how to pass the vertex data format to the vertex
/// shader once it's been uploaded into GPU memory
/// @param vertex
/// @return
VkVertexInputBindingDescription RenderUtils::GetBindingDescription() {
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding   = 0;
    bindingDesc.stride    = sizeof(Vertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Move to the next data entry after each
                                                         // vertex (for non instanced rendering)
    return bindingDesc;
}

/// @brief Creates an array of vertex attributes from a vertex originating from a binding
/// description
/// @param vertex
/// @return
std::array<VkVertexInputAttributeDescription, 2> RenderUtils::GetAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescs;

    // define vertex
    attributeDescs[0].binding  = 0;
    attributeDescs[0].location = 0;
    attributeDescs[0].format   = VK_FORMAT_R32G32_SFLOAT;
    attributeDescs[0].offset   = offsetof(Vertex, pos);

    // define color
    attributeDescs[1].binding  = 0;
    attributeDescs[1].location = 1;
    attributeDescs[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescs[1].offset   = offsetof(Vertex, color);

    return attributeDescs;
}
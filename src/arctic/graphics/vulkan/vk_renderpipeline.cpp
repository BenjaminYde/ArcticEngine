#include "vk_renderpipeline.h"
#include <format>
#include <iostream>

#include "arctic/core/utilities/application.h"
#include "arctic/core/utilities/file_utility.h"

#include "render_utils.h"

VulkanRenderPipeline::VulkanRenderPipeline(const VkDevice& vkDevice, uint32_t graphicsFamilyIndex,
                                           uint32_t transferFamilyIndex)
    : vkDevice(vkDevice), graphicsFamilyIndex(graphicsFamilyIndex),
      transferFamilyIndex(transferFamilyIndex) {}

void VulkanRenderPipeline::Load(const SwapChainData&            swapChainData,
                                const std::vector<VkImageView>& swapChainImageViews) {
    this->swapChainData       = swapChainData;
    this->swapChainImageViews = swapChainImageViews;

    createRenderPass();
    createPipeline();
    createFramebuffers();
}

void VulkanRenderPipeline::CleanUp() {
    // frame buffers
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(vkDevice, framebuffer, nullptr);
    }

    // pipeline
    vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, nullptr);
    vkDestroyPipeline(vkDevice, vkPipeline, nullptr);
    vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, nullptr);
    vkDestroyRenderPass(vkDevice, vkRenderPass, nullptr);
}

uint32_t VulkanRenderPipeline::GetGraphicsFamilyIndex() {
    return this->graphicsFamilyIndex;
}

uint32_t VulkanRenderPipeline::GetTransferFamilyIndex() {
    return this->transferFamilyIndex;
}

const VkRenderPass& VulkanRenderPipeline::GetRenderPass() {
    return this->vkRenderPass;
}

const VkPipeline& VulkanRenderPipeline::GetPipeline() {
    return this->vkPipeline;
}

const VkPipelineLayout& VulkanRenderPipeline::GetPipelineLayout() {
    return this->vkPipelineLayout;
}

const VkDescriptorSetLayout& VulkanRenderPipeline::GetDescriptorSetLayout() {
    return this->vkDescriptorSetLayout;
}

const VkFramebuffer& VulkanRenderPipeline::GetFrameBuffer(uint32_t index) {
    return this->swapChainFramebuffers[index];
}

/// <summary>
/// Specify the render pass and subpass index that the pipeline is compatible with,
/// This determines the framebuffer attachments, their formats, and the load/store operations
/// </summary>
void VulkanRenderPipeline::createRenderPass() {
    // create color attachment
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format  = swapChainData.imageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    // what to do before and after rendering
    //> clear to black before rendering
    colorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // currently application won't do anything with the stencil buffer
    // todo later: set correct values when need to use the stencil
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // images need to be transitioned to specific layouts that are suitable
    // for the operation that they're going to be involved in next
    // for example: textures and framebuffers in Vulkan are represented by 'VkImage' objects
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // create color attachment reference
    // every subpass references one or more attachment rederences
    //> only one is used
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // create sub pass
    //> the index of the attachment in this array is directly referenced from the fragment shader
    // with > "layout(location = 0) out vec4 outColor" directive
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorAttachmentRef;

    // create info: render pass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments    = &colorAttachment;
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;

    //> set subpass dependencies to render pass
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;

    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies   = &dependency;

    // create render pass
    VkResult resultPipeline = vkCreateRenderPass(vkDevice, &renderPassInfo, nullptr, &vkRenderPass);
    if (resultPipeline != VK_SUCCESS) {
        std::cout << "error: vulkan: failed to create render pass!";
        return;
    }
}

/// <summary>
/// This function creates 1 vulkan render pipeline using the following components
/// - shaderStages (vertex and frag stage info)
/// - vertexInputInfo (the format of the vertex data that will be passed to the vertex shader)
/// - inputAssembly (what kind of geometry/topology will be drawn from the vertices)
/// - viewportState (description of the viewport)
/// - pRasterizationState (takes the geometry that is shaped by the vertices from the vertex shader
/// and turns it into colored fragments, also performs depth testing, face culling)
/// - pDepthStencilState
/// - multisampling (ways to perform anti-aliasing)
/// - colorBlending (define how the final color values are blended with the existing values in the
/// framebuffer)
/// - dynamicState (specify which pipeline states can be changed dynamically during command buffer
/// recording without recreating the pipeline)
/// - vkPipelineLayout (define descriptor set layouts that describe the resource bindings used by
/// shaders (e.g., uniform buffers, textures, samplers))
///</summary>
void VulkanRenderPipeline::createPipeline() {
    // read file: vertex shader
    std::vector<char> fileVert;
    std::string       pathVert = std::format("{}/shaders/{}", Application::AssetsPath, "vert.spv");
    FileUtility::ReadBinaryFile(pathVert, fileVert);
    VkShaderModule shaderModuleVert;
    createShaderModule(fileVert, shaderModuleVert);

    // read file: frag shader
    std::vector<char> fileFrag;
    std::string       pathFrag = std::format("{}/shaders/{}", Application::AssetsPath, "frag.spv");
    FileUtility::ReadBinaryFile(pathFrag, fileFrag);
    VkShaderModule shaderModuleFrag;
    createShaderModule(fileFrag, shaderModuleFrag);

    // create vertex pipeline shader stage
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = shaderModuleVert;
    vertShaderStageInfo.pName  = "main";

    // create frag pipeline shader stage
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = shaderModuleFrag;
    fragShaderStageInfo.pName  = "main";

    // combine pipeline shader stages
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // create info: dynamic states
    //> while most of the pipeline state needs to be baked into the pipeline state,
    //> a limited amount of the state can actually be changed without recreating the pipeline at
    // draw time
    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                 VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates    = dynamicStates.data();

    // create info: create vertex input
    //> describes the format of the vertex data that will be passed to the vertex shader
    //> bindings: spacing between data and whether the data is per-vertex or per-instance
    //> attribute descriptions: type of the attributes passed to the vertex shader, which binding to
    // load them from and at which offset
    auto vertexBindingDesc    = RenderUtils::GetBindingDescription();
    auto vertexAttributeDescs = RenderUtils::GetAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions    = &vertexBindingDesc; // optional
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(vertexAttributeDescs.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescs.data(); // optional

    // create info: input assembly
    //> what kind of geometry/topology will be drawn from the vertices
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE; // no need for strips (for example terrain)

    // create viewport & scissor
    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (float)swapChainData.extent.width;
    viewport.height   = (float)swapChainData.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainData.extent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;

    // create info: rasterizer
    //> the rasterizer takes the geometry that is shaped by the vertices from the vertex shader
    //> and turns it into fragments to be colored by the fragment shader
    //> it also performs depth testing, face culling, ...
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;

    rasterizer.lineWidth = 1.0f;

    rasterizer.cullMode  = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    rasterizer.depthBiasEnable         = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // optional
    rasterizer.depthBiasClamp          = 0.0f; // optional
    rasterizer.depthBiasSlopeFactor    = 0.0f; // optional

    // create info: multi sampling
    //> one of the ways to perform anti-aliasing
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable   = VK_FALSE;
    multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading      = 1.0f;     // optional
    multisampling.pSampleMask           = nullptr;  // optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // optional
    multisampling.alphaToOneEnable      = VK_FALSE; // optional

    // create info: depth and stencil testing
    // todo
    //> for now pass a nullptr to be disabled

    // create color blend attachment state
    //> contains the configuration per attached framebuffer
    //> method: mix the old and new color value to produce a final color

    //> see pseudocode:
    /*
       finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
       finalColor.a = newAlpha.a;
    */

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                               VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable         = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

    // create info: color blend states
    //> contains the global color blending settings
    //> references the array of structures for all framebuffers and allows you to set blend
    // constants that you can use as blend factors in the calculations
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable     = VK_FALSE;
    colorBlending.logicOp           = VK_LOGIC_OP_COPY; // optional
    colorBlending.attachmentCount   = 1;
    colorBlending.pAttachments      = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // optional
    colorBlending.blendConstants[1] = 0.0f; // optional
    colorBlending.blendConstants[2] = 0.0f; // optional
    colorBlending.blendConstants[3] = 0.0f; // optional

    // create descriptor set layout
    createDescriptorSetLayout();

    // create info: pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = 1;
    pipelineLayoutInfo.pSetLayouts            = &this->vkDescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;       // optional
    pipelineLayoutInfo.pPushConstantRanges    = nullptr; // optional

    VkResult resultPipelineLayout =
        vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &vkPipelineLayout);
    if (resultPipelineLayout != VK_SUCCESS) {
        std::cout << "error: vulkan: failed to create pipeline layout!";
        return;
    }

    // create info: graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages    = shaderStages;

    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = nullptr; // optional
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;

    pipelineInfo.layout = vkPipelineLayout;

    pipelineInfo.renderPass = vkRenderPass;
    pipelineInfo.subpass    = 0;

    pipelineInfo.basePipelineHandle =
        VK_NULL_HANDLE;                  // optional: inherit from other pipelines (can be faster)
    pipelineInfo.basePipelineIndex = -1; // optional

    // info: it is designed to take multiple VkGraphicsPipelineCreateInfo objects and create
    // multiple VkPipeline objects in a single call
    VkResult resultPipeline =
        vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkPipeline);
    if (resultPipeline != VK_SUCCESS) {
        std::cout << "error: vulkan: failed to create pipeline!";
        return;
    }

    // cleanup shaders
    vkDestroyShaderModule(vkDevice, shaderModuleVert, nullptr);
    vkDestroyShaderModule(vkDevice, shaderModuleFrag, nullptr);
}

void VulkanRenderPipeline::createFramebuffers() {
    // set size of frame buffers
    swapChainFramebuffers.resize(swapChainImageViews.size());

    // loop over ALL swap chain image views
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {swapChainImageViews[i]};

        // create info: frame buffer
        //> you can only use a framebuffer with the render passes that it is compatible with
        //> which roughly means that they use the same number and type of attachments
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = vkRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments    = attachments;
        framebufferInfo.width           = swapChainData.extent.width;
        framebufferInfo.height          = swapChainData.extent.height;
        framebufferInfo.layers          = 1;

        // create frame buffer
        VkResult resultFrameBuffer =
            vkCreateFramebuffer(vkDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]);
        if (resultFrameBuffer != VK_SUCCESS) {
            std::cout << "error: vulkan: failed to create framebuffer!";
            return;
        }
    }
}

void VulkanRenderPipeline::createDescriptorSetLayout() {
    // create descriptor set layout binding: uniform buffer
    VkDescriptorSetLayoutBinding dslBinding{};
    dslBinding.binding         = 0; // binding index in the shader
    dslBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dslBinding.descriptorCount = 1;
    dslBinding.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT; // we're only referencing the descriptor from the vertex shader
    dslBinding.pImmutableSamplers =
        nullptr; // optional, only relevant for image sampling related descriptors

    // create descriptor set layout
    VkDescriptorSetLayoutCreateInfo dslInfo{};
    dslInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dslInfo.bindingCount = 1;
    dslInfo.pBindings    = &dslBinding;

    VkResult result = vkCreateDescriptorSetLayout(this->vkDevice, &dslInfo, nullptr,
                                                  &this->vkDescriptorSetLayout);
    if (result != VK_SUCCESS) {
        std::cout << "error: vulkan: failed to create descriptor set layout!";
        return;
    }
}

bool VulkanRenderPipeline::createShaderModule(const std::vector<char>& code,
                                              VkShaderModule&          shaderModule) {
    // create shader create info
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    // create shader
    VkResult result = vkCreateShaderModule(vkDevice, &createInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS) {
        std::cout << "error: vulkan: failed to create shader module!";
        return false;
    }
    return true;
}
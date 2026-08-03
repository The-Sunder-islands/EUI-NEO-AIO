#include "core/render/vulkan/vulkan_backend.h"

#include "core/render/vulkan/vulkan_canvas_shaders.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace core::render::vulkan {

namespace {

struct CanvasPushConstants {
    float windowSize[4] = {};      // x=windowWidth, y=windowHeight
    float fillColor[4] = {};
    float strokeColor[4] = {};
    float shapeRect[4] = {};       // x, y, w, h (or cx, cy, r, 0 for circles)
    float shapeParams[4] = {};     // x=cornerRadius, y=strokeWidth, z=shapeKind, w=unused
    float transform[4] = {};       // m00, m01, tx, ty
    float transform2[4] = {};      // m10, m11, 0, opacity
};

static_assert(sizeof(CanvasPushConstants) == 112, "Canvas push constants must fit Vulkan minimum size.");

} // namespace

void VulkanRenderBackend::drawCanvasShape(const CanvasDrawCommand& command,
                                           int windowWidth,
                                           int windowHeight) {
    if (!frameActive_ || windowWidth <= 0 || windowHeight <= 0 || command.opacity <= 0.001f) {
        return;
    }
    if (!ensureCanvasPipeline() || !ensurePrimitiveVertexBuffer(6)) {
        return;
    }
    if (!frameRecorded_) {
        recordClearPass(clearColor_);
    }
    if (!renderPassActive_) {
        return;
    }

    // Compute the bounding box that the quad must cover so the fragment shader's
    // SDF has enough pixels to rasterize. For strokes we expand by half the
    // stroke width; for circles we build a square around the center.
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
    const bool isStroke =
        command.kind == CanvasShapeKind::StrokeCircle ||
        command.kind == CanvasShapeKind::StrokeRect ||
        command.kind == CanvasShapeKind::StrokeRoundedRect ||
        command.kind == CanvasShapeKind::StrokePath ||
        command.kind == CanvasShapeKind::Line;
    const float strokeExtent = isStroke ? command.strokeWidth * 0.5f : 0.0f;

    if (command.kind == CanvasShapeKind::Line) {
        // rect.x,y = start; rect.width,height = end.
        minX = std::min(command.rect.x, command.rect.width) - strokeExtent;
        minY = std::min(command.rect.y, command.rect.height) - strokeExtent;
        maxX = std::max(command.rect.x, command.rect.width) + strokeExtent;
        maxY = std::max(command.rect.y, command.rect.height) + strokeExtent;
    } else if (command.kind == CanvasShapeKind::FillCircle ||
               command.kind == CanvasShapeKind::StrokeCircle) {
        const float radius = command.rect.width + strokeExtent;
        minX = command.rect.x - radius;
        minY = command.rect.y - radius;
        maxX = command.rect.x + radius;
        maxY = command.rect.y + radius;
    } else {
        minX = command.rect.x - strokeExtent;
        minY = command.rect.y - strokeExtent;
        maxX = command.rect.x + command.rect.width + strokeExtent;
        maxY = command.rect.y + command.rect.height + strokeExtent;
    }

    if (maxX <= minX || maxY <= minY) {
        return;
    }

    const std::size_t vertexOffset = primitiveVertices_.used;
    auto* mappedVertices = static_cast<PrimitiveGeometryVertex*>(primitiveVertices_.mapped);
    // Two triangles covering the bounding box. The screen attribute carries
    // the local (canvas-space) coordinates; the vertex shader applies the
    // push-constant transform to produce window-space positions.
    const float xs[6] = {minX, maxX, maxX, minX, maxX, minX};
    const float ys[6] = {minY, minY, maxY, minY, maxY, maxY};
    for (std::size_t i = 0; i < 6; ++i) {
        mappedVertices[vertexOffset + i].screen = {xs[i], ys[i], 1.0f};
        mappedVertices[vertexOffset + i].local = {xs[i], ys[i]};
    }
    primitiveVertices_.used += 6;

    VkCommandBuffer commandBuffer = currentCommandBuffer();
    if (!applyDrawViewportAndScissor(windowWidth, windowHeight)) {
        return;
    }

    CanvasPushConstants constants{};
    constants.windowSize[0] = static_cast<float>(windowWidth);
    constants.windowSize[1] = static_cast<float>(windowHeight);
    constants.windowSize[2] = 0.0f;
    constants.windowSize[3] = 0.0f;
    writeColor(constants.fillColor, command.fillColor);
    writeColor(constants.strokeColor, command.strokeColor);
    constants.shapeRect[0] = command.rect.x;
    constants.shapeRect[1] = command.rect.y;
    constants.shapeRect[2] = command.rect.width;
    constants.shapeRect[3] = command.rect.height;
    constants.shapeParams[0] = command.cornerRadius;
    constants.shapeParams[1] = command.strokeWidth;
    constants.shapeParams[2] = static_cast<float>(command.kind);
    constants.shapeParams[3] = 0.0f;
    constants.transform[0] = command.transform.m00;
    constants.transform[1] = command.transform.m01;
    constants.transform[2] = command.transform.tx;
    constants.transform[3] = command.transform.ty;
    constants.transform2[0] = command.transform.m10;
    constants.transform2[1] = command.transform.m11;
    constants.transform2[2] = 0.0f;
    constants.transform2[3] = command.opacity;

    const VkDeviceSize vertexBufferOffset = static_cast<VkDeviceSize>(vertexOffset * sizeof(PrimitiveGeometryVertex));
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, canvasPipeline_);
    if (canvasDescriptorSetLayout_ != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                canvasPipelineLayout_,
                                0,
                                0,
                                nullptr,
                                0,
                                nullptr);
    }
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &primitiveVertices_.buffer, &vertexBufferOffset);
    vkCmdPushConstants(commandBuffer,
                       canvasPipelineLayout_,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0,
                       sizeof(constants),
                       &constants);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
    ++core::render::currentRenderFrameStats().canvasDraws;
}

bool VulkanRenderBackend::ensureCanvasPipeline() {
    if (canvasPipeline_ != VK_NULL_HANDLE) {
        return true;
    }
    if (device_ == VK_NULL_HANDLE || renderPass_ == VK_NULL_HANDLE) {
        return false;
    }

    if (canvasDescriptorSetLayout_ == VK_NULL_HANDLE) {
        VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo{};
        descriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorLayoutInfo.bindingCount = 0;
        descriptorLayoutInfo.pBindings = nullptr;
        if (vkCreateDescriptorSetLayout(device_, &descriptorLayoutInfo, nullptr, &canvasDescriptorSetLayout_) != VK_SUCCESS) {
            return false;
        }
    }

    VkShaderModule vertexShader = createShaderModule(device_,
                                                     shaders::kCanvasVertexSpirv,
                                                     shaders::kCanvasVertexSpirvSize);
    VkShaderModule fragmentShader = createShaderModule(device_,
                                                       shaders::kCanvasFragmentSpirv,
                                                       shaders::kCanvasFragmentSpirvSize);
    if (vertexShader == VK_NULL_HANDLE || fragmentShader == VK_NULL_HANDLE) {
        if (vertexShader != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device_, vertexShader, nullptr);
        }
        if (fragmentShader != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device_, fragmentShader, nullptr);
        }
        return false;
    }

    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertexShader;
    shaderStages[0].pName = "main";
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragmentShader;
    shaderStages[1].pName = "main";

    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(PrimitiveGeometryVertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> attributes{};
    attributes[0].binding = 0;
    attributes[0].location = 0;
    attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[0].offset = offsetof(PrimitiveGeometryVertex, screen);
    attributes[1].binding = 0;
    attributes[1].location = 1;
    attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributes[1].offset = offsetof(PrimitiveGeometryVertex, local);

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributes.size());
    vertexInput.pVertexAttributeDescriptions = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                          VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT |
                                          VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::array<VkDynamicState, 2> dynamicStates{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(CanvasPushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &canvasDescriptorSetLayout_;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
    if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &canvasPipelineLayout_) != VK_SUCCESS) {
        vkDestroyShaderModule(device_, fragmentShader, nullptr);
        vkDestroyShaderModule(device_, vertexShader, nullptr);
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = canvasPipelineLayout_;
    pipelineInfo.renderPass = renderPass_;
    pipelineInfo.subpass = 0;

    const bool created = vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &canvasPipeline_) == VK_SUCCESS;
    vkDestroyShaderModule(device_, fragmentShader, nullptr);
    vkDestroyShaderModule(device_, vertexShader, nullptr);
    if (!created) {
        destroyCanvasPipeline();
    }
    return created;
}

void VulkanRenderBackend::destroyCanvasPipeline() {
    if (canvasPipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, canvasPipeline_, nullptr);
        canvasPipeline_ = VK_NULL_HANDLE;
    }
    if (canvasPipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, canvasPipelineLayout_, nullptr);
        canvasPipelineLayout_ = VK_NULL_HANDLE;
    }
    if (canvasDescriptorSetLayout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, canvasDescriptorSetLayout_, nullptr);
        canvasDescriptorSetLayout_ = VK_NULL_HANDLE;
    }
}

} // namespace core::render::vulkan

#include "pch.h"
#include "RasterGraphicsPipeline.hpp"


namespace VRTR
{
    RasterGraphicsPipeline::RasterGraphicsPipeline(Context& ctx) : ctx(ctx)
    {}

    void RasterGraphicsPipeline::createPipeline(vk::Format& format)
    {
        VRTR_DEBUG("Creating Raster Graphics Pipeline");

        // VBO

        vk::VertexInputBindingDescription bindingDescription
        {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex
        };

        std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions = 
        {{
            {
                .location = 0,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = offsetof(Vertex, pos)
            },
            {
                .location = 1,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = offsetof(Vertex, color)
            }
        }};

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo =
        {
            .pNext = nullptr,
            .flags = {},
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &bindingDescription,
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
            .pVertexAttributeDescriptions = attributeDescriptions.data()
        };

        // SHADERS
        std::vector<char> shaderCode = shaderHandler.readFile("shaders/slang.spv");
        vk::raii::ShaderModule shaderModule = shaderHandler.createShaderModule(ctx.logicalDevice, shaderCode);

        // PIPELINE CREATION

        // ==== SHADER STAGES ====
        // == VERTEX SHADER STAGE INFO ==
        vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
            .pNext = nullptr,
            .flags = {},
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = shaderModule,
            .pName = "vertMain",
            .pSpecializationInfo = {},
        };

        // == FRAGMENT SHADER STAGE INFO ==
        vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
            .pNext = nullptr,
            .flags = {},
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = shaderModule,
            .pName = "fragMain",
            .pSpecializationInfo = {},
        };

        std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages =
        {
            vertShaderStageInfo,
            fragShaderStageInfo
        };

        // ==== INPUT ASSEMBLY STAGE ====
        // "contains the configuration for what kind of topology will be drawn. 
        // This is where you set it to draw triangles, lines, points, or others like triangle-list."

        // vk::PipelineInputAssemblyStateCreateFlags inputAssemblyFlags = vk::PipelineInputAssemblyStateCreateFlags();

        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
            .pNext = nullptr,
            .flags = {},
            .topology = vk::PrimitiveTopology::eTriangleList,
            .primitiveRestartEnable = false
        };

        // ==== VIEWPORT AND SCISSOR STAGES ==== (???)

        std::array<vk::DynamicState, 2> dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };

        vk::PipelineDynamicStateCreateInfo dynamicStateInfo{
            .pNext = nullptr,
            .flags = {},
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data()
        };

        // It's not required since these parts are dynamic states
        // vk::Viewport viewport{
        //     .x = 0.0f,
        //     .y = 0.0f,
        //     .width = surCapabilities.currentExtent.width,
        //     .height = surCapabilities.currentExtent.height,
        //     .minDepth = 0.0f,
        //     .maxDepth = 1.0f
        // };

        // vk::Rect2D scissor{
        //     .offset = {0, 0},
        //     .extent = surCapabilities.currentExtent
        // };

        vk::PipelineViewportStateCreateInfo viewportStateInfo{
            .pNext = nullptr,
            .flags = {},
            .viewportCount = 1,
            .pViewports = nullptr, // Dynamic state
            .scissorCount = 1,
            .pScissors = nullptr // Dynamic state
        };

        // ==== RASTERIZATION STAGE ====
        // TODO
        vk::PipelineRasterizationStateCreateInfo rasterizationInfo
        {
            .pNext = nullptr,
            .flags = {},
            .depthClampEnable = vk::False,
            .rasterizerDiscardEnable = vk::False,
            .polygonMode = vk::PolygonMode::eFill,
            .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eClockwise,
            .depthBiasEnable = vk::False,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 1.0f,
            .lineWidth = 1.0f 
        };

        // ==== MULTISAMPLING STAGE ====
        // TODO
        vk::PipelineMultisampleStateCreateInfo multisampleInfo
        {
            .pNext = nullptr,
            .flags = {},
            .rasterizationSamples = vk::SampleCountFlagBits::e1,
            .sampleShadingEnable = false,
            .minSampleShading = 1.0f,
            .pSampleMask = nullptr,
            .alphaToCoverageEnable = false,
            .alphaToOneEnable = false
        };

        // ==== DEPTH AND STENCIL TESTING STAGE ====
        // TODO
        vk::PipelineDepthStencilStateCreateInfo depthStencilInfo
        {
            .pNext = nullptr,
            .flags = {},
            .depthTestEnable = false,
            .depthWriteEnable = false,
            .depthCompareOp = vk::CompareOp::eLess,
            .depthBoundsTestEnable = false,
            .stencilTestEnable = false,
            .front = {}, // Not used
            .back = {}, // Not used
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f
        };

        // ==== COLOR BLENDING STAGE ====
        // TODO
        vk::PipelineColorBlendAttachmentState colorBlendAttachmentInfo
        {
            .blendEnable = false,
            .srcColorBlendFactor = vk::BlendFactor::eZero,
            .dstColorBlendFactor = vk::BlendFactor::eZero,
            .colorBlendOp = vk::BlendOp::eAdd,
            .srcAlphaBlendFactor = vk::BlendFactor::eZero,
            .dstAlphaBlendFactor = vk::BlendFactor::eZero,
            .alphaBlendOp = vk::BlendOp::eAdd,
            .colorWriteMask = {
                vk::ColorComponentFlagBits::eR |
                vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eB |
                vk::ColorComponentFlagBits::eA
            }
        };

        std::array<float, 4> bc = {0.0f, 0.0f, 0.0f, 0.0f};

        vk::PipelineColorBlendStateCreateInfo colorBlendInfo
        {
            .pNext = nullptr,
            .flags = {},
            .logicOpEnable = vk::False,
            .logicOp = vk::LogicOp::eClear,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachmentInfo,
            .blendConstants = bc
        };

        // ==== PIPELINE LAYOUT ====
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo
        {
            .pNext = nullptr,
            .flags = {},
            .setLayoutCount = 0,
            .pSetLayouts = nullptr,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr
        };

        // I am using dynamic rendering that is above version 1.3,
        // so I don't need rederPass and framebuffer objects
        ctx.pipelineLayout = vk::raii::PipelineLayout(ctx.logicalDevice, pipelineLayoutInfo);

        vk::PipelineRenderingCreateInfo renderingInfo
        {
            .pNext = nullptr,
            .viewMask = 0,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &format,
            .depthAttachmentFormat = vk::Format::eUndefined,
            .stencilAttachmentFormat = vk::Format::eUndefined
        };

        vk::GraphicsPipelineCreateInfo pipelineInfo
        {
            .pNext = &renderingInfo,
            .flags = {},
            .stageCount = static_cast<uint32_t>(shaderStages.size()),
            .pStages = shaderStages.data(),
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssemblyInfo,
            .pTessellationState = nullptr,
            .pViewportState = &viewportStateInfo,
            .pRasterizationState = &rasterizationInfo,
            .pMultisampleState = &multisampleInfo,
            .pDepthStencilState = nullptr, // I dont use it now
            .pColorBlendState = &colorBlendInfo,
            .pDynamicState = &dynamicStateInfo,
            .layout = ctx.pipelineLayout,
            .renderPass = nullptr,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1
        };

        ctx.pipeline = vk::raii::Pipeline(ctx.logicalDevice, nullptr, pipelineInfo);
    }
}
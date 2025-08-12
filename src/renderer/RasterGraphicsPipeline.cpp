#include "pch.h"
#include "RasterGraphicsPipeline.hpp"


namespace VRTR
{
    void RasterGraphicsPipeline::createPipeline(vk::raii::Device& device)
        {
            // SHADERS
            std::vector<char> shaderCode = shaderHandler.readFile("shaders/slang.spv");
            vk::raii::ShaderModule shaderModule = shaderHandler.createShaderModule(device, shaderCode);

            // PIPELINE CREATION

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

        }
}
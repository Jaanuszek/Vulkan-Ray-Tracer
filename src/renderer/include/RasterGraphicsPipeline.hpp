#pragma once

#include "Logger.hpp"

namespace VRTR
{
    class RasterGraphicsPipeline
    {
        public:
            RasterGraphicsPipeline() = default;
            ~RasterGraphicsPipeline() = default;

        void createPipeline(vk::raii::Device& device, vk::raii::RenderPass& renderPass,
                            vk::raii::PipelineLayout& pipelineLayout, vk::raii::Pipeline& pipeline);
    };
}
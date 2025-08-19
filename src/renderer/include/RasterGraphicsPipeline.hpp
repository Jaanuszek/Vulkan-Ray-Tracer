#pragma once

#include "Logger.hpp"
#include "Shader.hpp"
#include "WindowSurface.hpp"

namespace VRTR
{
    class RasterGraphicsPipeline
    {
        public:
            RasterGraphicsPipeline();
            ~RasterGraphicsPipeline() = default;

        void createPipeline(vk::raii::Device& device, 
                            const SurfaceCapabilities& capabilities,
                            vk::raii::PipelineLayout& pipelineLayout,
                            vk::raii::Pipeline& pipeline);
            
        private:
            Shader shaderHandler;
            SurfaceCapabilities surCapabilities;
    };
}
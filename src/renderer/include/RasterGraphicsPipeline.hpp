#pragma once

#include "Logger.hpp"
#include "Shader.hpp"

namespace VRTR
{
    class RasterGraphicsPipeline
    {
        public:
            RasterGraphicsPipeline() = default;
            ~RasterGraphicsPipeline() = default;

        void createPipeline(vk::raii::Device& device);
            
        private:
            Shader shaderHandler;
    };
}
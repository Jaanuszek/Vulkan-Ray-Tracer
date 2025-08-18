#pragma once

#include "Logger.hpp"
#include "Shader.hpp"

namespace VRTR
{
    class RasterGraphicsPipeline
    {
        public:
            RasterGraphicsPipeline();
            ~RasterGraphicsPipeline() = default;

        void createPipeline(vk::raii::Device& device, 
                            const vk::SurfaceCapabilitiesKHR& capabilities);
            
        private:
            Shader shaderHandler;
            vk::SurfaceCapabilitiesKHR surCapabilities;
    };
}
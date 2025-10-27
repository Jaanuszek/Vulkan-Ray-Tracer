#pragma once

#include "Logger.hpp"
#include "Shader.hpp"
#include "ConstantsAndStructs.hpp"
// #include "SwapChainManager.hpp"

namespace VRTR
{
    class RasterGraphicsPipeline
    {
        public:
            RasterGraphicsPipeline(VULKAN_CONTEXT& ctx);
            ~RasterGraphicsPipeline() = default;

        void createPipeline(vk::Format& format);

        private:
            VULKAN_CONTEXT& ctx;
            Shader shaderHandler;
    };
}
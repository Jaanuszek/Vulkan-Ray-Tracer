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
            RasterGraphicsPipeline(Context& ctx);
            ~RasterGraphicsPipeline() = default;

        void createPipeline(vk::Format& format);

        private:
            Context& ctx;
            Shader shaderHandler;
    };
}
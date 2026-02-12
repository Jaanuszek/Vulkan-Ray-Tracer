#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"

namespace VRTR
{
    struct mesh
    {
        std::vector<VertexRT> vertices;
        std::vector<uint32_t> indices;
    };

    class ModelLoader
    {
        public:
            ModelLoader(RendererContext& ctx);
            mesh loadModel(const std::string &path);

        private:
            RendererContext& ctx;
    };
}

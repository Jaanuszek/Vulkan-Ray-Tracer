#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"
#include "Texture.hpp"

namespace VRTR
{
    struct mesh
    {
        std::vector<VertexRT> vertices;
        std::vector<uint32_t> indices;
    };

    class Model
    {
        public:
            Model(RendererContext& ctx, const std::string& modelPath, const std::string& texturePath);

            static std::string getModelNameFromPath(const std::string& path) { return std::filesystem::path(path).stem().string(); }

            std::string& getName() { return modelName; }
            mesh& getMesh() { return *modelMesh; }
            Texture& getTexture() { return *texture; }

        private:
            void loadModel(const std::string &path);
            void loadTexture(const std::string &path);

        private:
            RendererContext &ctx;

            std::string modelName;
            std::unique_ptr<mesh> modelMesh;
            std::unique_ptr<Texture> texture;
    };
}

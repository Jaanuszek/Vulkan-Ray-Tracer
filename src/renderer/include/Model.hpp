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

    enum class MaterialType
    {
        NONE = 0,
        ALBEDO = 1,
        METALLIC = 2,
        ROUGHNESS = 3
    };

    struct Material
    {
        glm::vec4 albedo;
        float metallic; // 4B
        float roughness; // 4B
        MaterialType type; // 4B
        float _pad;
    };

    // Struktura przechowująca device addresy buforów wierzchołków i indeksów
    // Przechowuje również indeks materiału, ktory bedzie uzyty w shaderach
    struct GeometryInfo
    {
        uint64_t vertexBufferAddr;
        uint64_t indexBufferAddr;
        uint32_t materialIndex;
    };

    class Model
    {
        public:
            Model(RendererContext& ctx, const std::string& modelPath, const std::string& texturePath);

            static std::string getModelNameFromPath(const std::string& path) { return std::filesystem::path(path).stem().string(); }

            void setMaterial(const Material& mat) { material = mat; }
            GeometryInfo getGeometryInfo() const { return geometryInfo; }

            void setGeometryInfo(const GeometryInfo& info) { geometryInfo = info; }

            std::string& getName() { return modelName; }
            mesh& getMesh() { return *modelMesh; }
            bool hasTexture() const { return withTexture; }
            Texture& getTexture() { return *texture; }

        private:
            void loadModel(const std::string &path);
            void loadTexture(const std::string &path);

        private:
            RendererContext &ctx;

            std::string modelName;
            std::unique_ptr<mesh> modelMesh;
            std::unique_ptr<Texture> texture;
            Material material;
            GeometryInfo geometryInfo;
            bool withTexture = false;
    };
}

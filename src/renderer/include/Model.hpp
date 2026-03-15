#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"
#include "Texture.hpp"
#include "VmaUsage.h"

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
        ROUGHNESS = 3,
        PBR = 4,
    };

    struct Material
    {
        glm::vec4 albedo;
        float metallic; // 4B
        float roughness; // 4B
        MaterialType type; // 4B
        uint32_t textureIndex;
    };

    // Struktura przechowująca device addresy buforów wierzchołków i indeksów
    // Przechowuje również indeks materiału, ktory bedzie uzyty w shaderach
    struct GeometryInfo
    {
        uint64_t vertexBufferAddr;
        uint64_t indexBufferAddr;
    };

    class Model
    {
        public:
            // temporary constructor, W przyszlosci pewnie informacje o materiale beda odczytywane z pliku modelu
            Model(RendererContext& ctx,
                    const std::string& modelPath, const std::string& texturePath,
                    const Material& mat = Material{});

            Model(RendererContext& ctx,
                    const std::vector<VertexRT>& vertices, const std::vector<uint32_t>& indices,
                    const Material& mat = Material{});

            ~Model();

            static std::string getModelNameFromPath(const std::string& path) { return std::filesystem::path(path).stem().string(); }

            void setMaterial(const Material& mat) { material = mat; }
            const GeometryInfo& getGeometryInfo() const { return geometryInfo; }
            Material getMaterial() const { return material; }

            std::string& getName() { return modelName; }

            mesh& getMesh() { return *modelMesh; }

            size_t getVertexCount() const { return modelMesh->vertices.size(); }
            size_t getIndexCount() const { return modelMesh->indices.size(); }

            vk::DeviceAddress getVertexBufferAddress() const { return geometryInfo.vertexBufferAddr; }
            vk::DeviceAddress getIndexBufferAddress() const { return geometryInfo.indexBufferAddr; }

            void setTextureIndex(uint32_t index) { material.textureIndex = index; }

            bool hasTexture() const { return withTexture; }

            Texture& getTexture() { return *texture; }

        private:
            void loadModel(const std::string &path);

            const VmaAllocationCreateInfo getVmaAllocCreateInfo();

            void createVertexBuffer();
            void createIndexBuffer();
            void setGeometryInfo();
            void loadTexture(const std::string &path);

        private:
            RendererContext &ctx;

            std::string modelName;
            std::unique_ptr<mesh> modelMesh;

            std::unique_ptr<Buffer> vertexBuffer;
            std::unique_ptr<Buffer> indexBuffer;

            std::unique_ptr<Texture> texture;
            Material material;
            GeometryInfo geometryInfo;
            bool withTexture = false;
    };

    namespace CustomModels
    {
        std::pair<std::vector<VertexRT>, std::vector<uint32_t>> createRectangle();
    }
}

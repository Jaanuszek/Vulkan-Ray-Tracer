#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"
#include "Texture.hpp"
#include "VmaUsage.h"
#include "CommonStructs.h"

#define enableRadiosity 1

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
        LIGHT = 5
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
        uint32_t triToPatchOffset;
        uint32_t triangleCount;
        uint64_t vertexBufferAddr;
        uint64_t indexBufferAddr;
    };

    class Model
    {
        public:
            // temporary constructor, W przyszlosci pewnie informacje o materiale beda odczytywane z pliku modelu
            Model(RendererContext& ctx,
                    const std::string& modelPath, const std::string& texturePath,
                    const Material& mat = Material{}, const glm::mat4& transform = glm::mat4(1.0f));

            Model(RendererContext& ctx,
                    const std::vector<VertexRT>& vertices, const std::vector<uint32_t>& indices,
                    const Material& mat = Material{}, const glm::mat4& transform = glm::mat4(1.0f));

            ~Model();

            static std::string getModelNameFromPath(const std::string& path) { return std::filesystem::path(path).stem().string(); }

            // void setMaterial(const Material& mat) { material = mat; }
            const GeometryInfo& getGeometryInfo() const { return geometryInfo; }
            Material getMaterial() const { return material; }

            std::string& getName() { return modelName; }

            // mesh& getMesh() { return *modelMesh; }
            const std::vector<Patch>& getPatches() const { return patches; }
            const std::vector<uint32_t>& getPatchIdToTriangleId() const { return patchIdToTriangleId; }

            const uint32_t getTriangleCount() const { return triCount; }

            size_t getVertexCount() const { return modelMesh->vertices.size(); }
            size_t getIndexCount() const { return modelMesh->indices.size(); }

            const std::vector<uint32_t>& getLocalVertexToPatchIds() const { return vertexPatchIndices; }
            const std::vector<uint32_t>& getLocalVertexToPatchOffsets() const { return vertexPatchOffsets; }

            vk::DeviceAddress getVertexBufferAddress() const { return geometryInfo.vertexBufferAddr; }
            vk::DeviceAddress getIndexBufferAddress() const { return geometryInfo.indexBufferAddr; }

            void setTextureIndex(uint32_t index) { material.textureIndex = index; }

            bool hasTexture() const { return withTexture; }

            Texture& getTexture() { return *texture; }

            void rebuildPatches(uint8_t patchSize) { buildPatches(patchSize); }

        private:
            void loadModel(const std::string &path);

            const VmaAllocationCreateInfo getVmaAllocCreateInfo();

            void createVertexBuffer();
            void createIndexBuffer();
            void setGeometryInfo();
            void loadTexture(const std::string &path);
            void tessellateLargeTriangles(float maxWorldTriangleArea = 0.5f, uint32_t maxDepth = 6);
            void buildPatches(uint8_t patchSize = 1);
            void removeDuplicateVertices();
            void buildVertexPatchAdjacency();

        private:
            RendererContext &ctx;

            uint32_t triCount{};
            std::string modelName;
            std::unique_ptr<mesh> modelMesh;
            glm::mat4 modelTransform{1.0f};

            std::unique_ptr<Buffer> vertexBuffer;
            std::unique_ptr<Buffer> indexBuffer;

            std::vector<Patch> patches;

            // Kontener mapujący indeks trójkąta na indeks patcha do którego należy.
            std::vector<uint32_t> patchIdToTriangleId;

            // Struktura ktora mapuje indeks wierzchołka na listę patchy
            // potrzebne do interpolacji kolorów wierzchołków
            std::vector<std::vector<uint32_t>> vertexToPatchIds;


            // GPU friendly kontenery ktore robia to samo co vvertexToPatchIds
            std::vector<uint32_t> vertexPatchOffsets;
            std::vector<uint32_t> vertexPatchIndices;

            std::unique_ptr<Texture> texture;
            Material material;
            GeometryInfo geometryInfo;
            bool withTexture = false;
    };

    namespace CustomModels
    {
        std::pair<std::vector<VertexRT>, std::vector<uint32_t>> createRectangle(const glm::vec3& color = glm::vec3(0.0f));
        std::pair<std::vector<VertexRT>, std::vector<uint32_t>> createCube(const glm::vec3& color = glm::vec3(0.0f));
    }
}

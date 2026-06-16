#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"
#include "Texture.hpp"
#include "VmaUsage.h"
#include "CommonStructs.h"

#define enableRadiosity 1

namespace VRTR
{
    constexpr float EPSILON = 1e-6;

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
        LIGHT = 5,
        REFRACTION = 6,
    };

    struct Material
    {
        glm::vec4 albedo = glm::vec4(0.0f);
        glm::vec3 emission = glm::vec3(0.0f);
        float metallic = 0.0f; // 4B
        float roughness = 1.0f; // 4B
        MaterialType type = MaterialType::ALBEDO; // 4B
        uint32_t textureIndex = 0;
    };

    // Struktura przechowująca device addresy buforów wierzchołków i indeksów
    // Przechowuje również indeks materiału, ktory bedzie uzyty w shaderach
    struct GeometryInfo
    {
        uint32_t triToPatchOffset;
        uint32_t triangleCount;
        uint32_t vertexGlobalOffset;
        uint32_t triToMaterialOffset;
        uint64_t vertexBufferAddr;
        uint64_t indexBufferAddr;
    };

    struct VertexKey
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec2 tex;

        bool operator==(const VertexKey& other) const
        {
            return glm::all(glm::epsilonEqual(pos, other.pos, EPSILON)) &&
                   glm::all(glm::epsilonEqual(normal, other.normal, EPSILON)) &&
                   glm::all(glm::epsilonEqual(color, other.color, EPSILON)) &&
                   glm::all(glm::epsilonEqual(tex, other.tex, EPSILON));
        }
    };

    struct VertexKeyHash
    {
        size_t operator()(const VertexKey& k) const
        {
            size_t h1 = std::hash<float>{}(k.pos.x);
            size_t h2 = std::hash<float>{}(k.pos.y);
            size_t h3 = std::hash<float>{}(k.pos.z);

            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };

    class Model
    {
        public:
            // temporary constructor, W przyszlosci pewnie informacje o materiale beda odczytywane z pliku modelu
            Model(RendererContext& ctx,
                    const std::string& modelPath, 
                    const std::string& texturePath,
                    const glm::mat4& transform = glm::mat4(1.0f));

            Model(RendererContext& ctx,
                    const std::vector<VertexRT>& vertices,
                    const std::vector<uint32_t>& indices,
                    const std::vector<Material>& mats, 
                    const glm::mat4& transform = glm::mat4(1.0f));

            ~Model();

            static std::string getModelNameFromPath(const std::string& path) { return std::filesystem::path(path).stem().string(); }

            const GeometryInfo& getGeometryInfo() const { return geometryInfo; }

            std::vector<Material>& getMaterials() { return materials; }
            std::vector<uint32_t>& getTriIdxToMaterialIdx() { return triangleIdToMaterialId; }

            const std::vector<Patch>& getPatches() const { return patches; }
            const std::vector<uint32_t>& getPatchIdToTriangleId() const { return patchIdToTriangleId; }

            const uint32_t getTriangleCount() const { return triCount; }

            size_t getVertexCount() const { return modelMesh->vertices.size(); }
            size_t getIndexCount() const { return modelMesh->indices.size(); }

            const std::vector<uint32_t>& getLocalVertexToPatchIds() const { return vertexPatchIndices; }
            const std::vector<uint32_t>& getLocalVertexToPatchOffsets() const { return vertexPatchOffsets; }

            vk::DeviceAddress getVertexBufferAddress() const { return geometryInfo.vertexBufferAddr; }
            vk::DeviceAddress getIndexBufferAddress() const { return geometryInfo.indexBufferAddr; }

            void setTextureIndex(uint32_t index);

            bool hasTexture() const { return withTexture; }

            Texture& getTexture() { return *texture; }

            void rebuildPatches(uint8_t patchSize)
            {
                buildPatches(patchSize);
                removeDuplicateVertices();
                buildVertexPatchAdjacency();
            }

        private:
            void loadModel(const std::string &path);

            const VmaAllocationCreateInfo getVmaAllocCreateInfo();

            void createVertexBuffer();
            void createIndexBuffer();
            void setGeometryInfo();
            void loadTexture(const std::string &path);
            void buildPatches(uint8_t patchSize = 1);
            void weldVertices();
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

            std::vector<uint32_t> triangleIdToMaterialId;

            std::unique_ptr<Texture> texture;
            // Materialy dla kazdego face
            std::vector<Material> materials;
            GeometryInfo geometryInfo;
            bool withTexture = false;
            bool loadedFromFile = false;
            bool vertexInterpolation = true;
    };

    namespace CustomModels
    {
        void appendGridPlane(std::vector<VertexRT> &vertices,
                             std::vector<uint32_t> &indices,
                             const glm::vec3 &center,
                             const glm::vec3 &uAxis,
                             const glm::vec3 &vAxis,
                             const glm::vec3 &normal,
                             uint32_t uSegments,
                             uint32_t vSegments,
                             float uSize,
                             float vSize,
                             const glm::vec3 &color);

        std::pair<std::vector<VertexRT>, std::vector<uint32_t>> createDividedRectangle(
            uint32_t xSegments,
            uint32_t zSegments,
            float width,
            float depth,
            const glm::vec3 &color);

        std::pair<std::vector<VertexRT>, std::vector<uint32_t>> createDividedCube(
            uint32_t xSegments,
            uint32_t ySegments,
            uint32_t zSegments,
            float size,
            const glm::vec3 &color);

        std::pair<std::vector<VertexRT>, std::vector<uint32_t>> createRectangle(const glm::vec3 &color = glm::vec3(0.0f));
        std::pair<std::vector<VertexRT>, std::vector<uint32_t>> createCube(const glm::vec3& color = glm::vec3(0.0f));
        std::pair<std::vector<VertexRT>, std::vector<uint32_t>> createSphere(uint32_t stackCount = 24, uint32_t sectorCount = 48, float radius = 1.0f, const glm::vec3& color = glm::vec3(0.0f));
    }

}

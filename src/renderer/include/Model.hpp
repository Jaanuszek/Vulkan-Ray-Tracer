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
    };

    struct ModelBuffers
    {
        // nie wiem
        // Wydaje mi sie ze takie czyszczenie zasobo jest lepsze niz w deestruktorze klasy Model
        ModelBuffers(VmaAllocator& vmaAlloc) : vmaAlloc(vmaAlloc) {}

        ~ModelBuffers(){
            vmaFreeMemory(vmaAlloc, vertexAllocation);
            vmaFreeMemory(vmaAlloc, indexAllocation);
        }

        VmaAllocator vmaAlloc;

        vk::raii::Buffer vertexBuffer{nullptr};
        vk::raii::Buffer indexBuffer{nullptr};
        VmaAllocation vertexAllocation;
        VmaAllocation indexAllocation;
    };

    class Model
    {
        public:
            Model(RendererContext& ctx, VmaAllocator& vmaAlloc, const std::string& modelPath, const std::string& texturePath);
            ~Model();

            static std::string getModelNameFromPath(const std::string& path) { return std::filesystem::path(path).stem().string(); }

            void setMaterial(const Material& mat) { material = mat; }
            GeometryInfo getGeometryInfo() const { return geometryInfo; }

            std::string& getName() { return modelName; }

            mesh& getMesh() { return *modelMesh; }

            size_t getVertexCount() const { return modelMesh->vertices.size(); }
            size_t getIndexCount() const { return modelMesh->indices.size(); }

            vk::DeviceAddress getVertexBufferAddress() const { return geometryInfo.vertexBufferAddr; }
            vk::DeviceAddress getIndexBufferAddress() const { return geometryInfo.indexBufferAddr; }

            const ModelBuffers& getBuffers() { return modelBuffers; }

            // Wiem ze sie powtarzam, ale dla czytelnosci takie cos zrobie
            const vk::raii::Buffer& getVertexBuffer() const { return modelBuffers.vertexBuffer; }
            const vk::raii::Buffer& getIndexBuffer() const { return modelBuffers.indexBuffer; }

            const 

            bool hasTexture() const { return withTexture; }

            Texture& getTexture() { return *texture; }

        private:
            void loadModel(const std::string &path);

            // It's only applicable for creating vertex and index buffers
            vk::BufferCreateInfo getBufferCreateInfo(vk::DeviceSize size);
            VmaAllocationCreateInfo getVmaAllocCreateInfo();

            void createVertexBuffer();
            void createIndexBuffer();
            void setGeometryInfo();
            void loadTexture(const std::string &path);

        private:
            RendererContext &ctx;

            std::string modelName;
            std::unique_ptr<mesh> modelMesh;

            ModelBuffers modelBuffers;

            std::unique_ptr<Texture> texture;
            Material material;
            GeometryInfo geometryInfo;
            bool withTexture = false;
    };
}

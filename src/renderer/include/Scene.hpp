#pragma once
#include "Logger.hpp"
#include "AccelerationStructureManager.hpp"
#include "Model.hpp"
#include "DescriptorManager.hpp"
#include "StorageBuffer.hpp"

/*
    Ta klasa bedzie przechowywala wszystkie obiekty znajdujące się na scenie
    Będzie umożliwiała dodawanie nowych modeli
    Będzie przechowywala Acceleration Structure, ponieważ ta struktura przechowuje informacje o geometrii, 
    czyli w zasadzie o scenie
*/

const std::string LIGHT_MODEL_NAME = "light_object";

namespace VRTR
{
    class Scene
    {
        public:
            Scene(RendererContext& ctx);
            ~Scene();

            void createScene(const glm::vec3& lightPos);

            void updateTLAS(const float& rotationAngle);

            void updateInstanceTLAS(uint32_t instanceIdx, const glm::mat4& newTransform);

            void appendDescriptorResources(DescriptorResources& resources);

            uint32_t getLightTLASIdx() const { return lightSourceTLASIdx; }
            const GeometryInfo& getGeometryInfo(const std::string& modelName) const { return models.at(modelName)->getGeometryInfo(); }

            std::vector<Patch>& getPatches() { return patchesGlobal; }
            uint32_t getPatchCount() const { return static_cast<uint32_t>(patchesGlobal.size()); }

            void updatePatchData(uint8_t patchSize);

        private:
            uint32_t importModel(const std::string &modelPath, const std::string &texPath);

            uint32_t addObject(const std::string& objName, const std::vector<VertexRT>& vertices, 
                    const std::vector<uint32_t>& indices, const Material& mat,
                    const glm::mat4& transform);

            void buildTLAS();

            void fillSSBOContainers();

        private:
            RendererContext& ctx;

            std::unique_ptr<AccelerationStructureManager> asManager;

            std::unordered_map<std::string, std::unique_ptr<Model>> models;
            std::vector<std::string> modelInstanceOrder;

            /* 
                Zmienna ktora przechowuje aktualny indeks tekstury ktory bedzie przypisany 
                do nastepnego modelu z tekstura jaki zostanie dodany do sceny
            */
            uint32_t textureIndexCounter = 0;
            uint32_t lightSourceTLASIdx{0}; // póki co zakladam ze mamy tylko jedno źródło światła

            std::vector<uint32_t> triToPatchGlobal;
            std::vector<Patch> patchesGlobal;

            std::vector<GeometryInfo> geometryInfos;
            std::vector<Material> materials;

            std::unique_ptr<StorageBuffer> geometrySBO;
            std::unique_ptr<StorageBuffer> materialSBO;
            std::unique_ptr<StorageBuffer> triToPatchBuffer;
            std::unique_ptr<StorageBuffer> patchBuffer;
    };
}
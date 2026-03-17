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

            void importModel(const std::string &modelPath, const std::string &texPath);

            // TODO na 100% da sie to lepiej rozwiazac niz nazywanie kazdego obiektu w jakis sposob
            // ale moze to tez jest dobre?
            void addObject(const std::string& objName, const std::vector<VertexRT>& vertices, 
                            const std::vector<uint32_t>& indices, const Material& mat,
                            const glm::mat4& transform);

            // Wrapper do zbudowania TLASu, zeby nie trzeba bylo expose'owac całego ASManagera
            void buildTLAS();

            // Wrapper do aktualizacji TLASu (transformacji obiektów)
            void updateTLAS(float deltaTime, const float& rotationAngle);

            void appendDescriptorResources(DescriptorResources& resources);

            const GeometryInfo& getGeometryInfo(const std::string& modelName) const { return models.at(modelName)->getGeometryInfo(); }

        private:
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

            std::vector<uint32_t> triToPatchGlobal;
            std::vector<Patch> patchesGlobal;

            std::vector<GeometryInfo> geometryInfos;
            std::vector<Material> materials;

            std::unique_ptr<StorageBuffer> geometrySBO;
            std::unique_ptr<StorageBuffer> materialSBO;
            std::unique_ptr<StorageBuffer> triToPatchBuffer;
    };
}
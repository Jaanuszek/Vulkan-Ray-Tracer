#pragma once
#include "Logger.hpp"
#include "AccelerationStructureManager.hpp"
#include "Model.hpp"
#include "DescriptorManager.hpp"

/*
    Ta klasa bedzie przechowywala wszystkie obiekty znajdujące się na scenie
    Będzie umożliwiała dodawanie nowych modeli
    Będzie przechowywala Acceleration Structure, ponieważ ta struktura przechowuje informacje o geometrii, 
    czyli w zasadzie o scenie
*/

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

            /*
                Ta funkcja updatuje tylko ImagieView i Samplery modeli
            */
            void updateDescriptorResources(DescriptorResources& resources);

            vk::ImageView getModelTextureImageView(const std::string &name) const { return models.at(name)->getTexture().getTextureImageViewHandle(); }
            vk::Sampler getModelTextureSampler(const std::string &name) const { return models.at(name)->getTexture().getTextureSamplerHandle(); }

            // For descriptor set
            const vk::AccelerationStructureKHR getTLASHandle() const { return asManager->getTLASHandle(); }

            const std::vector<GeometryInfo>& getGeometryInfos() const { return geometryInfos; }
            const GeometryInfo& getGeometryInfo(const std::string& modelName) const { return models.at(modelName)->getGeometryInfo(); }
            const std::vector<Material>& getMaterials() const { return materials; }

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

            /* 
                TODO mysle ze to poniżej ma sens (?)
                Kontenery przechowujace dane ktore beda przesylane do GPU przez SBO
            */
            std::vector<GeometryInfo> geometryInfos;
            std::vector<Material> materials;
    };
}
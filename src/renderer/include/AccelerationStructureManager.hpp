#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"
#include "Utils.hpp"
#include "buffer.hpp"
#include "CommandBufferManager.hpp"
#include "Model.hpp"

namespace VRTR
{
    struct AccelerationStructure
    {
        vk::raii::AccelerationStructureKHR handle{nullptr};
        vk::DeviceAddress deviceAddress{0};
        std::unique_ptr<Buffer> asBuffer{nullptr};
        std::unique_ptr<Buffer> scratchBuffer{nullptr};
    };

    // Dla czytelnosci. Bo mnie nazwa Acceleration Structure myli
    using BottomLevelAS = AccelerationStructure;

    struct TopLevelAS
    {
        AccelerationStructure as;
        std::unique_ptr<Buffer> instanceBuffer{nullptr};
        uint32_t instanceCount{0};
    };

    class AccelerationStructureManager
    {
        public:
            AccelerationStructureManager(RendererContext& ctx);

            /*
             Funkcja ta tworzy bufory wierzchołków i indeksów dla modelu
             Tworzy Blas, zapisuje go do vektora ze wszystkimi blasami
             Zwraca indeks tego Blasa w wektorze blasów (jest to po prostu indeks tego Blasa w wektorze blasów)
            */
            uint32_t createBLAS(std::unique_ptr<Model>& model);

            void buildTLAS();

            void addInstance(uint32_t blasIdx, const glm::mat4 &transform);

            void updateTLAS(float deltaTime, const float& rotationAngle);

            const vk::raii::AccelerationStructureKHR& getTLAS() const { return tlas.as.handle; }

            vk::AccelerationStructureKHR getTLASHandle() const { return *tlas.as.handle; }

            BottomLevelAS& getBLAS(uint32_t index) { return blasList.at(index); }


        private:
        void primitiveToGeometry(std::unique_ptr<Model>& model,
                                 vk::AccelerationStructureGeometryKHR &geometry,
                                 vk::AccelerationStructureBuildRangeInfoKHR &offsetInfo,
                                 vk::Format vertexFormat = vk::Format::eR32G32B32Sfloat,
                                 vk::IndexType indexType = vk::IndexType::eUint32);

        void createAccelerationStructure(vk::AccelerationStructureTypeKHR asType,
                                         AccelerationStructure &as,
                                         vk::AccelerationStructureGeometryKHR &asGeometry,
                                         vk::AccelerationStructureBuildRangeInfoKHR &asBuildRangeInfo,
                                         vk::BuildAccelerationStructureFlagsKHR flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);

        private:
            RendererContext& ctx;

            std::vector<vk::AccelerationStructureInstanceKHR> vkInstances; // Przechowuje opis instancji
            std::unordered_map<uint32_t, glm::mat4> instanceTransforms; // Przechowuje domyslne maceirze transformacji uzyte podczas tworzenia modelu

            std::vector<BottomLevelAS> blasList;
            TopLevelAS tlas;
    };
}
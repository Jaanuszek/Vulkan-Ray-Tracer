#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"
#include "Utils.hpp"
#include "buffer.hpp"
#include "CommandBufferManager.hpp"

namespace VRTR
{
    struct AccelerationStructure
    {
        vk::raii::AccelerationStructureKHR handle{nullptr};
        vk::DeviceAddress deviceAddress{0};
        std::unique_ptr<Buffer> asBuffer{nullptr};
        std::unique_ptr<Buffer> scratchBuffer{nullptr};
    };

    struct BottomLevelAS
    {
        AccelerationStructure as;
        std::unique_ptr<Buffer> vertexBuffer{nullptr};
        std::unique_ptr<Buffer> indexBuffer{nullptr};
    };

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
        
            uint32_t createBLAS(const std::vector<VertexRT>& vertices,
                                const std::vector<uint32_t>& indices);

            void buildTLAS();

            void addInstance(uint32_t blasIdx, const glm::mat4 &transform);

            void updateTLAS(float deltaTime);

            vk::raii::AccelerationStructureKHR& getTLAS() { return tlas.as.handle; }

            BottomLevelAS& getBLAS(uint32_t index) { return blasList.at(index); }


        private:
        void primitiveToGeometry(const std::vector<VertexRT> &vertices,
                                 const std::vector<uint32_t> &indices,
                                 std::unique_ptr<Buffer> &vertexBuffer,
                                 std::unique_ptr<Buffer> &indexBuffer,
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

            std::vector<BottomLevelAS> blasList;
            TopLevelAS tlas;
    };
}
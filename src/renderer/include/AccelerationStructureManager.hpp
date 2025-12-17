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

    struct InstanceData
    {
        glm::mat4 transform;
        uint32_t blasIdx;
        uint32_t customIdx;
        uint32_t mask;
        uint32_t hitGroupIndex; // used to fetch the shaders from the SBT
    };

    class AccelerationStructureManager
    {
        public:
            AccelerationStructureManager(VULKAN_CONTEXT& ctx);
        
            uint32_t createBLAS(const std::vector<VertexRT>& vertices,
                                const std::vector<uint32_t>& indices);

            void buildTLAS();

            void addInstance(uint32_t blasIdx, const glm::mat4 &transform);

            void updateTLAS(const glm::mat4& transform);

            vk::raii::AccelerationStructureKHR& getTLAS() { return tlas.as.handle; }

        private:
        void primitiveToGeometry(const std::vector<VertexRT> &vertices,
                                 const std::vector<uint32_t> &indices,
                                 std::unique_ptr<Buffer> &vertexBuffer,
                                 std::unique_ptr<Buffer> &indexBuffer,
                                 vk::AccelerationStructureGeometryKHR &geometry,
                                 vk::AccelerationStructureBuildRangeInfoKHR &offsetInfo,
                                 vk::Format vertexFormat = vk::Format::eR32G32B32Sfloat,
                                 vk::IndexType indexType = vk::IndexType::eUint32);

        void createAccelerationStructure(VRTR::VULKAN_CONTEXT &ctx,
                                         vk::AccelerationStructureTypeKHR asType,
                                         AccelerationStructure &as,
                                         vk::AccelerationStructureGeometryKHR &asGeometry,
                                         vk::AccelerationStructureBuildRangeInfoKHR &asBuildRangeInfo,
                                         vk::BuildAccelerationStructureFlagsKHR flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);

        private:
            VULKAN_CONTEXT& ctx;
            // TODO change to RendererContext

            // Instances data  blasID, transofmr, mask, customidx, hitGroupIndex
            std::vector<InstanceData> instances;

            std::vector<BottomLevelAS> blasList;
            TopLevelAS tlas;
    };
}
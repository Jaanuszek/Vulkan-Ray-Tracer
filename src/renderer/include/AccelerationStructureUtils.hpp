#pragma once
#include <Logger.hpp>
#include "ConstantsAndStructs.hpp"
#include "Utils.hpp"
#include "CommandBufferManager.hpp"
#include "buffer.hpp"

namespace VRTR
{
    // Acceleration Structure helper functions
    namespace AS
    {
        struct AccelerationStructure
        {
            vk::raii::AccelerationStructureKHR handle{nullptr};
            vk::DeviceAddress deviceAddress{0};
            std::unique_ptr<Buffer> asBuffer{nullptr};
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

        // W przyszłości dodac tu moze jakąs templatke, zeby mozna bylo dawac rozne struktury, nie tylko VertexRT
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
                                         AS::AccelerationStructure &as,
                                         vk::AccelerationStructureGeometryKHR &asGeometry,
                                         vk::AccelerationStructureBuildRangeInfoKHR &asBuildRangeInfo,
                                         vk::BuildAccelerationStructureFlagsKHR flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
    }
}
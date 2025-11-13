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
                                         VRTR::AccelerationStructure &as,
                                         vk::AccelerationStructureGeometryKHR &asGeometry,
                                         vk::AccelerationStructureBuildRangeInfoKHR &asBuildRangeInfo,
                                         vk::BuildAccelerationStructureFlagsKHR flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
    }
}
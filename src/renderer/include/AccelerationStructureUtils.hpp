#pragma once
#include <Logger.hpp>
#include "ConstantsAndStructs.hpp"

namespace VRTR
{
    // Acceleration Structure helper functions
    namespace AS
    {
        // W przyszłości dodac tu moze jakąs templatke, zeby mozna bylo dawac rozne struktury, nie tylko VertexRT
        void primitiveToGeometry(const std::vector<VertexRT>& vertices,
                                 const std::vector<uint32_t>& indices,
                                 std::shared_ptr<primitiveBuffers> buffers,
                                 vk::AccelerationStructureGeometryKHR& geometry,
                                 vk::AccelerationStructureBuildRangeInfoKHR& offsetInfo,
                                 vk::Format vertexFormat = vk::Format::eR32G32B32Sfloat,
                                 vk::IndexType indexType = vk::IndexType::eUint32);

        void createAccelerationStructure(Context& ctx,
                                         vk::AccelerationStructureTypeKHR asType,
                                         VRTR::AccelerationStructure& as,
                                         vk::AccelerationStructureGeometryKHR& asGeometry,
                                         vk::AccelerationStructureBuildRangeInfoKHR& asBuildRangeInfo,
                                         vk::BuildAccelerationStructureFlagsKHR flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
    }
}
#include "pch.h"
#include "AccelerationStructureUtils.hpp"

void VRTR::AS::primitiveToGeometry(const std::vector<VertexRT>& vertices,
                                 const std::vector<uint32_t>& indices,
                                 std::shared_ptr<primitiveBuffers> buffers,
                                 vk::AccelerationStructureGeometryKHR& geometry,
                                 vk::AccelerationStructureBuildRangeInfoKHR& offsetInfo,
                                 vk::Format vertexFormat,
                                 vk::IndexType indexType)
{
    uint32_t triangleCount = static_cast<uint32_t>(indices.size() / 3U);

    vk::AccelerationStructureGeometryTrianglesDataKHR triangles
    {
        .pNext = nullptr,
        .vertexFormat = vertexFormat,
        .vertexData = buffers->vertexBuffer->getDeviceAddress(),
        .vertexStride = sizeof(VertexRT),
        .maxVertex = static_cast<uint32_t>(vertices.size() - 1),
        .indexType = indexType,
        .indexData = buffers->indexBuffer->getDeviceAddress(),
        .transformData = {}
    };

    geometry = vk::AccelerationStructureGeometryKHR
    {
        .pNext = nullptr,
        .geometryType = vk::GeometryTypeKHR::eTriangles,
        .geometry = triangles,
        .flags = vk::GeometryFlagBitsKHR::eOpaque | vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation
    };

    offsetInfo = vk::AccelerationStructureBuildRangeInfoKHR
    {
        .primitiveCount = triangleCount,
        .primitiveOffset = 0,
        .firstVertex = 0,
        .transformOffset = 0
    };
}

void VRTR::AS::createAccelerationStructure(Context& ctx,
                                           vk::AccelerationStructureTypeKHR asType,
                                           VRTR::AccelerationStructure& as,
                                           vk::AccelerationStructureGeometryKHR& asGeometry,
                                           vk::AccelerationStructureBuildRangeInfoKHR& asBuildRangeInfo,
                                           vk::BuildAccelerationStructureFlagsKHR flags)
{
    vk::AccelerationStructureBuildGeometryInfoKHR asBuildInfo
    {
        .type = asType,
        .flags = flags,
        .mode = vk::BuildAccelerationStructureModeKHR::eBuild,
        .geometryCount = 1,
        .pGeometries = &asGeometry,
    };
    std::vector<uint32_t> maxPrimCount(1);
    maxPrimCount.at(0) = asBuildRangeInfo.primitiveCount;

    vk::AccelerationStructureBuildSizesInfoKHR asSizeInfo = ctx.logicalDevice.getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice,
        asBuildInfo,
        maxPrimCount
    );

    vk::DeviceSize scratchSize = asSizeInfo.buildScratchSize;
    vk::DeviceSize minAsScratchOffsetAlignment = ctx.asProperties.minAccelerationStructureScratchOffsetAlignment;
}
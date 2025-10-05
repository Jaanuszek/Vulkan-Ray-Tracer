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
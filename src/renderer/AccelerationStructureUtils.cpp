#include "pch.h"
#include "AccelerationStructureUtils.hpp"

void VRTR::AS::primitiveToGeometry(const std::vector<VertexRT> &vertices,
                                   const std::vector<uint32_t> &indices,
                                   std::shared_ptr<primitiveBuffers> buffers,
                                   vk::AccelerationStructureGeometryKHR &geometry,
                                   vk::AccelerationStructureBuildRangeInfoKHR &offsetInfo,
                                   vk::Format vertexFormat,
                                   vk::IndexType indexType)
{
    uint32_t triangleCount = static_cast<uint32_t>(indices.size() / 3U);

    vk::AccelerationStructureGeometryTrianglesDataKHR triangles{
        .pNext = nullptr,
        .vertexFormat = vertexFormat,
        .vertexData = vk::DeviceOrHostAddressConstKHR{buffers->vertexBuffer->getDeviceAddress()},
        .vertexStride = sizeof(VertexRT),
        .maxVertex = static_cast<uint32_t>(vertices.size() - 1),
        .indexType = indexType,
        .indexData = vk::DeviceOrHostAddressConstKHR{buffers->indexBuffer->getDeviceAddress()},
        .transformData = {}};

    geometry = vk::AccelerationStructureGeometryKHR{
        .pNext = nullptr,
        .geometryType = vk::GeometryTypeKHR::eTriangles,
        .geometry = triangles,
        .flags = vk::GeometryFlagBitsKHR::eOpaque | vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation};

    offsetInfo = vk::AccelerationStructureBuildRangeInfoKHR{
        .primitiveCount = triangleCount,
        .primitiveOffset = 0,
        .firstVertex = 0,
        .transformOffset = 0};
}

void VRTR::AS::createAccelerationStructure(VRTR::VULKAN_CONTEXT &ctx,
                                           vk::AccelerationStructureTypeKHR asType,
                                           VRTR::AccelerationStructure &as,
                                           vk::AccelerationStructureGeometryKHR &asGeometry,
                                           vk::AccelerationStructureBuildRangeInfoKHR &asBuildRangeInfo,
                                           vk::BuildAccelerationStructureFlagsKHR flags)
{
    // gemoetry data
    vk::AccelerationStructureBuildGeometryInfoKHR asBuildInfo{
        .type = asType,
        .flags = flags,
        .mode = vk::BuildAccelerationStructureModeKHR::eBuild,
        .geometryCount = 1,
        .pGeometries = &asGeometry,
    };

    std::vector<uint32_t> maxPrimCount(1);
    maxPrimCount.at(0) = asBuildRangeInfo.primitiveCount;

    // getting size info
    // size needed for the acceleration structure itself,
    // size needed for the scratch buffer
    // size needed for the update scratch buffer
    vk::AccelerationStructureBuildSizesInfoKHR asSizeInfo = ctx.logicalDevice.getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice,
        asBuildInfo,
        maxPrimCount);

    vk::DeviceSize scratchSize = asSizeInfo.buildScratchSize;
    vk::DeviceSize minAsScratchOffsetAlignment = ctx.asProperties.minAccelerationStructureScratchOffsetAlignment;
    scratchSize = utils::aligned_size(scratchSize, minAsScratchOffsetAlignment);

    Buffer scratchBuffer(ctx, BufferType::SCRATCH, scratchSize);

    // we need also a buffer that will hold the acceleration structure
    as.buffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, asSizeInfo.accelerationStructureSize,
                                         vk::BufferUsageFlags{}, vk::MemoryPropertyFlagBits::eDeviceLocal,
                                         vk::BufferUsageFlagBits2::eAccelerationStructureStorageKHR |
                                             vk::BufferUsageFlagBits2::eShaderDeviceAddress);

    vk::AccelerationStructureCreateInfoKHR asCreateInfo{
        .pNext = nullptr,
        .createFlags = {},
        .buffer = as.buffer->getBuffer(),
        .offset = 0,
        .size = asSizeInfo.accelerationStructureSize,
        .type = asType,
        .deviceAddress = 0};
    as.handle = vk::raii::AccelerationStructureKHR(ctx.logicalDevice, asCreateInfo);

    // temp cmd buffer
    auto tempCmdBuffer = CommandBuffer::createTempCommandBuffer(ctx, vk::CommandBufferLevel::ePrimary, true);

    asBuildInfo.dstAccelerationStructure = *as.handle;
    asBuildInfo.scratchData = scratchBuffer.getDeviceAddress();

    std::array<vk::AccelerationStructureBuildRangeInfoKHR *, 1> BuildRangeInfos = {&asBuildRangeInfo};
    tempCmdBuffer.buildAccelerationStructuresKHR({asBuildInfo}, BuildRangeInfos);

    CommandBuffer::flushTempCommandBuffer(ctx, tempCmdBuffer);

    as.device_address = ctx.logicalDevice.getAccelerationStructureAddressKHR(
        vk::AccelerationStructureDeviceAddressInfoKHR{
            .accelerationStructure = *as.handle});
}
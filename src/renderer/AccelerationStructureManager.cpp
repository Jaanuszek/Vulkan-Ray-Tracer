#include "pch.h"
#include "AccelerationStructureManager.hpp"

namespace VRTR
{
    AccelerationStructureManager::AccelerationStructureManager()
    {
    }

    uint32_t AccelerationStructureManager::createBLAS(const std::vector<VertexRT>& vertices,
                                                     const std::vector<uint32_t>& indices)
    {
        VRTR_DEBUG("Creating BLAS");

        AS::BottomLevelAS blas_structure;

        size_t vertex_buffer_size = vertices.size() * sizeof(VertexRT);
        size_t index_buffer_size = indices.size() * sizeof(uint32_t);

        const vk::BufferUsageFlags2 buffer_usage_flags = vk::BufferUsageFlagBits2::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits2::eShaderDeviceAddress;
        const vk::MemoryPropertyFlags memory_property_flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

        blas_structure.vertexBuffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, vertex_buffer_size, vk::BufferUsageFlags{}, memory_property_flags, buffer_usage_flags);
        blas_structure.vertexBuffer->Update(vertices.data(), vertex_buffer_size);

        blas_structure.indexBuffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, index_buffer_size, vk::BufferUsageFlags{}, memory_property_flags, buffer_usage_flags);
        blas_structure.indexBuffer->Update(indices.data(), index_buffer_size);

        vk::AccelerationStructureGeometryKHR asGeometry{};
        vk::AccelerationStructureBuildRangeInfoKHR offsetInfo{};
        AS::primitiveToGeometry(vertices, indices, blas_structure.vertexBuffer, blas_structure.indexBuffer, asGeometry, offsetInfo);
        // inicjacja struktury acceleration structure.

        AS::createAccelerationStructure(ctx,
                                        vk::AccelerationStructureTypeKHR::eBottomLevel,
                                        blas_structure.as,
                                        asGeometry,
                                        offsetInfo,
                                        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
        blasList.push_back(std::move(blas_structure));
        return static_cast<uint32_t>(blasList.size() - 1);
    }

    void AccelerationStructureManager::buildTLAS()
    {
        VRTR_DEBUG("Creating TLAS");

        std::vector<vk::AccelerationStructureInstanceKHR> vkInstances;
        instances.reserve(instances.size());
        // Propably I would like to save it to a variable
        // vk::TransformMatrixKHR transformMatrix{
        //     std::array<std::array<float, 4>, 3>{
        //         1.0f, 0.0f, 0.0f, 0.0f,
        //         0.0f, 1.0f, 0.0f, 0.0f,
        //         0.0f, 0.0f, 1.0f, 0.0f}};

        for (const auto& inst : instances)
        {
            vk::TransformMatrixKHR transform{};
            memcpy(&transform, &inst.transform, sizeof(glm::mat4));
            vk::AccelerationStructureInstanceKHR ac_instance{
                .transform = transform,
                .instanceCustomIndex = inst.customIdx,
                .mask = inst.mask,
                .instanceShaderBindingTableRecordOffset = inst.hitGroupIndex,
                .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
                .accelerationStructureReference = blasList[inst.blasIdx].as.deviceAddress
            };

            vkInstances.push_back(ac_instance);
        }

        const vk::DeviceSize instanceBufferSize = vkInstances.size() * sizeof(vk::AccelerationStructureInstanceKHR);

        tlas.instanceBuffer = std::make_unique<Buffer>(ctx.logicalDevice,
                                                        ctx.gpu,
                                                        sizeof(vk::AccelerationStructureInstanceKHR),
                                                        vk::BufferUsageFlags{},
                                                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                                        vk::BufferUsageFlagBits2::eShaderDeviceAddress | vk::BufferUsageFlagBits2::eAccelerationStructureBuildInputReadOnlyKHR);

        tlas.instanceBuffer->Update(vkInstances.data(), instanceBufferSize);

        vk::DeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
        instanceDataDeviceAddress.deviceAddress = tlas.instanceBuffer->getDeviceAddress();

        vk::AccelerationStructureGeometryKHR ASGeometry{
            .pNext = nullptr,
            .geometryType = vk::GeometryTypeKHR::eInstances,
            .geometry = vk::AccelerationStructureGeometryInstancesDataKHR{
                .pNext = nullptr,
                .arrayOfPointers = VK_FALSE,
                .data = instanceDataDeviceAddress},
            .flags = vk::GeometryFlagBitsKHR::eOpaque};

        vk::AccelerationStructureBuildRangeInfoKHR ASBuildRangeInfo{
            .primitiveCount = tlas.instanceCount,// jak dodam wiecej obiektow to to trzeba zamienic na ilosc instancji
            .primitiveOffset = 0,
            .firstVertex = 0,
            .transformOffset = 0};

        AS::createAccelerationStructure(ctx,
                                        vk::AccelerationStructureTypeKHR::eTopLevel,
                                        tlas.as,
                                        ASGeometry,
                                        ASBuildRangeInfo,
                                        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
    }

    void AccelerationStructureManager::addInstance(uint32_t blasIdx, const glm::mat4 &transform)
    {
    }

    void AccelerationStructureManager::updateTLAS()
    {
    }
}
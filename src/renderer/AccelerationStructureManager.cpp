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

        // TODO DODAC BUFFORY DO STRUKTURY ACCELERATIONSTRUCTURE
        // VERTEX BUFFER INDEX BUFFER


        // std::vector<VertexRT> verticesRT = {
        //     {{1.0f, 1.0f, 0.0f}},
        //     {{-1.0f, 1.0f, 0.0f}},
        //     {{0.0f, -1.0f, 0.0f}}};
        // std::vector<uint32_t> indicesRT = {0, 1, 2};

        size_t vertex_buffer_size = vertices.size() * sizeof(VertexRT);
        size_t index_buffer_size = indices.size() * sizeof(uint32_t);

        const vk::BufferUsageFlags2 buffer_usage_flags = vk::BufferUsageFlagBits2::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits2::eShaderDeviceAddress;
        const vk::MemoryPropertyFlags memory_property_flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

        vertex_buffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, vertex_buffer_size, vk::BufferUsageFlags{}, memory_property_flags, buffer_usage_flags);
        vertex_buffer->Update(verticesRT.data(), vertex_buffer_size);

        index_buffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, index_buffer_size, vk::BufferUsageFlags{}, memory_property_flags, buffer_usage_flags);
        index_buffer->Update(indicesRT.data(), index_buffer_size);

        vk::AccelerationStructureGeometryKHR asGeometry{};
        vk::AccelerationStructureBuildRangeInfoKHR offsetInfo{};
        AS::primitiveToGeometry(verticesRT, indicesRT, vertex_buffer, index_buffer, asGeometry, offsetInfo);
        // inicjacja struktury acceleration structure.

        AS::createAccelerationStructure(ctx,
                                        vk::AccelerationStructureTypeKHR::eBottomLevel,
                                        blas_structure,
                                        asGeometry,
                                        offsetInfo,
                                        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
        blasList.push_back(std::move(blas_structure));
        return static_cast<uint32_t>(blasList.size() - 1);
    }

    void AccelerationStructureManager::buildTLAS()
    {
    }

    void AccelerationStructureManager::addInstance(uint32_t blasIdx, const glm::mat4 &transform)
    {
    }

    void AccelerationStructureManager::updateTLAS()
    {
    }
}
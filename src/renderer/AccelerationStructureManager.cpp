#include "pch.h"
#include "AccelerationStructureManager.hpp"

namespace VRTR
{
    AccelerationStructureManager::AccelerationStructureManager(RendererContext& ctx)
        : ctx(ctx)
    {
    }

    uint32_t AccelerationStructureManager::createBLAS(std::unique_ptr<Model>& model)
    {
        VRTR_DEBUG("Creating BLAS");

        BottomLevelAS blas_structure;

        vk::AccelerationStructureGeometryKHR asGeometry{};
        vk::AccelerationStructureBuildRangeInfoKHR offsetInfo{};
        primitiveToGeometry(model, asGeometry, offsetInfo);
        // inicjacja struktury acceleration structure.
        createAccelerationStructure(vk::AccelerationStructureTypeKHR::eBottomLevel,
                                    blas_structure,
                                    asGeometry,
                                    offsetInfo,
                                    vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);

        blasList.push_back(std::move(blas_structure));
        return static_cast<uint32_t>(blasList.size() - 1);
    }

    void AccelerationStructureManager::buildTLAS()
    {
        VRTR_DEBUG("Creating TLAS");

        const vk::DeviceSize instanceBufferSize = vkInstances.size() * sizeof(vk::AccelerationStructureInstanceKHR);
        if(instanceBufferSize == 0)
        {
            VRTR_WARN("No instances to build TLAS");
            return;
        }

        tlas.instanceBuffer = std::make_unique<Buffer>(ctx.logicalDevice,
                                                        ctx.gpu,
                                                        instanceBufferSize,
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

        createAccelerationStructure(vk::AccelerationStructureTypeKHR::eTopLevel,
                                    tlas.as,
                                    ASGeometry,
                                    ASBuildRangeInfo,
                                    vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate);
    }

    void AccelerationStructureManager::addInstance(uint32_t blasIdx, const glm::mat4 &transform)
    {
        vk::TransformMatrixKHR transformMatrix{};

        // ogolnie Vulkan w instancji AS przyjmuje maceirz 3x4 row major
        // glm jest column major, wiec trzeba transponowac, zeby to sie zgadzalo
        // nie trzeba tego przepisywac na oddzielna strukture 3x4,
        glm::mat4 transposedMat = glm::transpose(transform);

        // tutaj po prostu pomijamy 4 rząd macierzy glm::mat4
        memcpy(&transformMatrix, &transposedMat, sizeof(vk::TransformMatrixKHR)); // TODO jak bedzie cos nie tak to zmienic na sizeof(glm::mat4)

        vk::AccelerationStructureInstanceKHR ac_instance{
            .transform = transformMatrix,
            .instanceCustomIndex = static_cast<uint32_t>(vkInstances.size()), // tu nie moze byc blasIdx,, bo mozemy miec duzo tlasow odnoszacych sie do tego samego blasa
            .mask = 0xFF,
            .instanceShaderBindingTableRecordOffset = 0, // temp
            .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
            .accelerationStructureReference = blasList[blasIdx].deviceAddress};
        vkInstances.push_back(ac_instance);

        instanceTransforms[vkInstances.size() - 1] = transposedMat; // -1 bo robimy push_back przed tym, wiec indekst to rozmiar - 1

        tlas.instanceCount = static_cast<uint32_t>(vkInstances.size());
    }

    void AccelerationStructureManager::updateTLAS(float deltaTime, const float& rotationAngle)
    {        
        for (size_t instIdx = 0; instIdx < vkInstances.size(); instIdx++)
        { 
            if (instanceTransforms.find(instIdx) == instanceTransforms.end())
                continue;
            
            // rotacja zeby przejsc z koordynatow blendera na vulkanowe
            glm::mat4 baseRotation = instanceTransforms[instIdx];

            glm::mat4 userRotation = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));

            glm::mat4 rotatedMat = baseRotation * userRotation;
            vk::TransformMatrixKHR transformMatrix{};
            memcpy(&transformMatrix, &rotatedMat, sizeof(vk::TransformMatrixKHR));

            vkInstances[instIdx].transform = transformMatrix;
        }

        tlas.instanceBuffer->Update(vkInstances.data(), vkInstances.size() * sizeof(vk::AccelerationStructureInstanceKHR));

        auto instancesData = vk::AccelerationStructureGeometryInstancesDataKHR{
            .arrayOfPointers = vk::False,
            .data = tlas.instanceBuffer->getDeviceAddress()
        };

        vk::AccelerationStructureGeometryDataKHR geometryData(instancesData);

        vk::AccelerationStructureGeometryKHR tlasGeometry{
            .geometryType = vk::GeometryTypeKHR::eInstances, // czy eTriangles?
            .geometry = geometryData,
            .flags = vk::GeometryFlagBitsKHR::eOpaque
        };

        vk::AccelerationStructureBuildGeometryInfoKHR tlasBuildGeometryInfo{
            .type = vk::AccelerationStructureTypeKHR::eTopLevel,
            .flags = vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate,
            .mode = vk::BuildAccelerationStructureModeKHR::eUpdate,
            .srcAccelerationStructure = tlas.as.handle,
            .dstAccelerationStructure = tlas.as.handle,
            .geometryCount = 1,
            .pGeometries = &tlasGeometry};

        vk::BufferDeviceAddressInfo scratchAddressInfo{
            .buffer = tlas.as.scratchBuffer->getBuffer()
        };
        vk::DeviceAddress scratchAddress = ctx.logicalDevice.getBufferAddress(scratchAddressInfo);
        tlasBuildGeometryInfo.scratchData.deviceAddress = scratchAddress;

        vk::AccelerationStructureBuildRangeInfoKHR tlasRangeInfo{
            .primitiveCount = tlas.instanceCount,
            .primitiveOffset = 0,
            .firstVertex = 0,
            .transformOffset = 0
        };

        std::unique_ptr<TempCMDBufferManager> tempCmdBufferManager = std::make_unique<TempCMDBufferManager>(ctx.logicalDevice, ctx.queue, ctx.graphics_queue_index);
        vk::raii::CommandBuffer& tempCmdBuffer = tempCmdBufferManager->createTempCmdBuffer();

        vk::MemoryBarrier preBarrier{
            .srcAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteKHR | vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eShaderRead,
            .dstAccessMask = vk::AccessFlagBits::eAccelerationStructureReadKHR | vk::AccessFlagBits::eAccelerationStructureWriteKHR};

        tempCmdBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR | vk::PipelineStageFlagBits::eTransfer | vk::PipelineStageFlagBits::eFragmentShader,
            vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
            {},
            preBarrier,
            {},
            {});

        tempCmdBuffer.buildAccelerationStructuresKHR(
            {tlasBuildGeometryInfo},
            {&tlasRangeInfo}
        );

        vk::MemoryBarrier postBarrier{
            .srcAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteKHR,
            .dstAccessMask = vk::AccessFlagBits::eAccelerationStructureReadKHR | vk::AccessFlagBits::eShaderRead};

        tempCmdBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
            vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR | vk::PipelineStageFlagBits::eFragmentShader,
            {},
            postBarrier,
            {},
            {});
        
        tempCmdBufferManager->submitAndWaitTempCmdBuffer();
    }

    void AccelerationStructureManager::primitiveToGeometry(
                                std::unique_ptr<Model>& model,
                                vk::AccelerationStructureGeometryKHR &geometry,
                                vk::AccelerationStructureBuildRangeInfoKHR &offsetInfo,
                                vk::Format vertexFormat,
                                vk::IndexType indexType)
    {
        uint32_t triangleCount = static_cast<uint32_t>(model->getIndexCount() / 3U);

        vk::AccelerationStructureGeometryTrianglesDataKHR triangles{
            .pNext = nullptr,
            .vertexFormat = vertexFormat,
            .vertexData = vk::DeviceOrHostAddressConstKHR{model->getVertexBufferAddress()},
            .vertexStride = sizeof(VertexRT),
            .maxVertex = static_cast<uint32_t>(model->getVertexCount() - 1),
            .indexType = indexType,
            .indexData = vk::DeviceOrHostAddressConstKHR{model->getIndexBufferAddress()},
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

    void AccelerationStructureManager::createAccelerationStructure(vk::AccelerationStructureTypeKHR asType,
                                            AccelerationStructure &as,
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
        vk::DeviceSize minAsScratchOffsetAlignment = ctx.properties.asProperties.minAccelerationStructureScratchOffsetAlignment;
        scratchSize = utils::aligned_size(scratchSize, minAsScratchOffsetAlignment);

        as.scratchBuffer = std::make_unique<Buffer>(ctx, BufferType::SCRATCH, scratchSize);

        // we need also a buffer that will hold the acceleration structure
        as.asBuffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, asSizeInfo.accelerationStructureSize,
                                            vk::BufferUsageFlags{}, vk::MemoryPropertyFlagBits::eDeviceLocal,
                                            vk::BufferUsageFlagBits2::eAccelerationStructureStorageKHR |
                                                vk::BufferUsageFlagBits2::eShaderDeviceAddress);

        vk::AccelerationStructureCreateInfoKHR asCreateInfo{
            .pNext = nullptr,
            .createFlags = {},
            .buffer = as.asBuffer->getBuffer(),
            .offset = 0,
            .size = asSizeInfo.accelerationStructureSize,
            .type = asType,
            .deviceAddress = 0};
        as.handle = vk::raii::AccelerationStructureKHR(ctx.logicalDevice, asCreateInfo);

        asBuildInfo.dstAccelerationStructure = *as.handle;
        asBuildInfo.scratchData = as.scratchBuffer->getDeviceAddress();

        std::unique_ptr<TempCMDBufferManager> tempCmdBufferManager = std::make_unique<TempCMDBufferManager>(ctx.logicalDevice, ctx.queue, ctx.graphics_queue_index);
        vk::raii::CommandBuffer& tempCmdBuffer = tempCmdBufferManager->createTempCmdBuffer();

        std::array<vk::AccelerationStructureBuildRangeInfoKHR *, 1> BuildRangeInfos = {&asBuildRangeInfo};
        tempCmdBuffer.buildAccelerationStructuresKHR({asBuildInfo}, BuildRangeInfos);

        as.deviceAddress = ctx.logicalDevice.getAccelerationStructureAddressKHR(
            vk::AccelerationStructureDeviceAddressInfoKHR{
                .accelerationStructure = *as.handle});
        tempCmdBufferManager->submitAndWaitTempCmdBuffer();
    }

}
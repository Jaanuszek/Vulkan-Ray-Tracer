#include "pch.h"
#include "VisibilityPipeline.hpp"
#include "Radiosity.cuh"

namespace VRTR
{
    VisibilityPipeline::VisibilityPipeline(RendererContext& ctx) : ctx(ctx)
    {}

    VisibilityPipeline::~VisibilityPipeline()
    {
        VRTR_DEBUG("Destroying VisibilityPipeline");
    }

    void VisibilityPipeline::init(DescriptorResources& resources, std::shared_ptr<CommandBufferManager>& commandBufferManager)
    {
        this->commandBufferManager = commandBufferManager;
        visibilityOutputBuffer = resources.cudaColorBuffer;

        descriptorManager = std::make_unique<VisibilityDescriptorManager>(ctx);
        descriptorManager->init(resources);

        createPipelineLayout();
        createShaderStages();
        createPipeline();
        createSBT();
        recordCommandBuffer();
    }

    void VisibilityPipeline::createPipelineLayout()
    {
        VRTR_DEBUG("Creating Visibility Pipeline");

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
            .flags = {},
            .setLayoutCount = 1,
            .pSetLayouts = &(*descriptorManager->getDescriptorSetLayout()),
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr
        };

        pipelineLayout = vk::raii::PipelineLayout(ctx.logicalDevice, pipelineLayoutInfo);
    }

    void VisibilityPipeline::createShaderStages()
    {
        shaderStages.push_back(shader.createShaderStageInfo(ctx.logicalDevice, CONSTANTS::SHADERS_DIR / "visibilityPass.spv", vk::ShaderStageFlagBits::eRaygenKHR, "rayGenShader"));
        vk::RayTracingShaderGroupCreateInfoKHR raygenGroup{
            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
            .generalShader = 0, // first entry in shaderStages
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR};
        shaderGroups.push_back(raygenGroup);

        // Miss shader
        shaderStages.push_back(shader.createShaderStageInfo(ctx.logicalDevice, CONSTANTS::SHADERS_DIR / "visibilityPass.spv", vk::ShaderStageFlagBits::eMissKHR, "missShader"));
        vk::RayTracingShaderGroupCreateInfoKHR missGroup{
            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
            .generalShader = 1, // second entry in shaderStages
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR};
        shaderGroups.push_back(missGroup);

        // Closest hit shader
        shaderStages.push_back(shader.createShaderStageInfo(ctx.logicalDevice, CONSTANTS::SHADERS_DIR / "visibilityPass.spv", vk::ShaderStageFlagBits::eClosestHitKHR, "closestHitShader"));
        shaderStages.push_back(shader.createShaderStageInfo(ctx.logicalDevice, CONSTANTS::SHADERS_DIR / "visibilityPass.spv", vk::ShaderStageFlagBits::eAnyHitKHR, "anyHitShader"));
        vk::RayTracingShaderGroupCreateInfoKHR hitGroup{
            .type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
            .generalShader = VK_SHADER_UNUSED_KHR,
            .closestHitShader = 2, // third entry in shaderStages
            .anyHitShader = 3, // fourth entry in shaderStages
            .intersectionShader = VK_SHADER_UNUSED_KHR};
        shaderGroups.push_back(hitGroup);
    }

    void VisibilityPipeline::createPipeline()
    {
        vk::RayTracingPipelineCreateInfoKHR pipelineInfo{
            .pNext = nullptr,
            .flags = {},
            .stageCount = static_cast<uint32_t>(shaderStages.size()),
            .pStages = shaderStages.data(),
            .groupCount = static_cast<uint32_t>(shaderGroups.size()),
            .pGroups = shaderGroups.data(),
            .maxPipelineRayRecursionDepth = 4,
            .pLibraryInfo = nullptr,
            .pLibraryInterface = nullptr,
            .pDynamicState = nullptr,
            .layout = pipelineLayout,
            .basePipelineHandle = nullptr,
            .basePipelineIndex = -1};

        pipeline = ctx.logicalDevice.createRayTracingPipelineKHR(nullptr, nullptr, pipelineInfo, nullptr);
    }

    void VisibilityPipeline::createSBT()
    {
        VRTR_DEBUG("Creating Shader Binding Table");
        const uint32_t handle_size = ctx.properties.rtPipelineProperties.shaderGroupHandleSize; // rozmiar jednego shader group
        const uint32_t handle_alignment = ctx.properties.rtPipelineProperties.shaderGroupHandleAlignment;
        const uint32_t handle_size_aligned = utils::aligned_size(handle_size, handle_alignment); // rozmiar wyrownania
        const uint32_t group_count = static_cast<uint32_t>(shaderGroups.size());                 // licza shaderow
        const uint32_t sbt_size = group_count * handle_size_aligned;                             // calkowity rozmiar SBT - ile bajtow potrzeba zeby zmieniscic wszystkie uchryty shaderow
        const vk::BufferUsageFlags sbt_buffer_usage_flags = vk::BufferUsageFlagBits::eShaderBindingTableKHR |
                                                            vk::BufferUsageFlagBits::eTransferSrc |
                                                            vk::BufferUsageFlagBits::eShaderDeviceAddress;
        const vk::MemoryPropertyFlags sbt_memory_property_flags = vk::MemoryPropertyFlagBits::eHostVisible |
                                                                  vk::MemoryPropertyFlagBits::eHostCoherent;

        raygen_shader_binding_table = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, handle_size, sbt_buffer_usage_flags, sbt_memory_property_flags);
        miss_shader_binding_table = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, handle_size, sbt_buffer_usage_flags, sbt_memory_property_flags);
        hit_shader_binding_table = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, handle_size, sbt_buffer_usage_flags, sbt_memory_property_flags);
        std::vector<uint8_t> shader_handle_storage(sbt_size);
        shader_handle_storage = pipeline.getRayTracingShaderGroupHandlesKHR<uint8_t>(0, group_count, sbt_size);

        // RAYGEN shader
        uint8_t *data = static_cast<uint8_t *>(raygen_shader_binding_table->map(handle_size, 0));
        memcpy(data, shader_handle_storage.data(), handle_size);
        raygen_shader_binding_table->unmap();
        // MISS shader
        data = static_cast<uint8_t *>(miss_shader_binding_table->map(handle_size, 0));
        memcpy(data, shader_handle_storage.data() + handle_size_aligned, handle_size);
        miss_shader_binding_table->unmap();

        // HIT shader
        data = static_cast<uint8_t *>(hit_shader_binding_table->map(handle_size, 0));
        memcpy(data, shader_handle_storage.data() + 2 * handle_size_aligned, handle_size);
        hit_shader_binding_table->unmap();
    }

    void VisibilityPipeline::recordCommandBuffer()
    {
        VRTR_DEBUG("Building Visibility Command Buffers");
        vk::CommandBufferBeginInfo beginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse,
            .pInheritanceInfo = nullptr};

        auto &commandBuffers = commandBufferManager->getVisibilityCommandBuffers();

        if (commandBuffers.empty()) {
            VRTR_CRITICAL("No command buffers allocated for visibility pipeline");
        }

        // for (uint32_t i = 0; i < commandBuffers.size(); i++)
        for(auto& commandBuffer : commandBuffers)
        {
            commandBuffer.begin(beginInfo);
            
            const uint32_t handle_size = ctx.properties.rtPipelineProperties.shaderGroupHandleSize;
            const uint32_t handle_alignment = ctx.properties.rtPipelineProperties.shaderGroupHandleAlignment;
            const uint32_t handle_size_aligned = utils::aligned_size(handle_size, handle_alignment);

            vk::StridedDeviceAddressRegionKHR raygenShaderSBTEntry{
                .deviceAddress = raygen_shader_binding_table->getDeviceAddress(),
                .stride = handle_size_aligned,
                .size = handle_size_aligned};

            vk::StridedDeviceAddressRegionKHR missShaderSBTEntry{
                .deviceAddress = miss_shader_binding_table->getDeviceAddress(),
                .stride = handle_size_aligned,
                .size = handle_size_aligned};

            vk::StridedDeviceAddressRegionKHR hitShaderSBTEntry{
                .deviceAddress = hit_shader_binding_table->getDeviceAddress(),
                .stride = handle_size_aligned,
                .size = handle_size_aligned};

            vk::StridedDeviceAddressRegionKHR callableShaderSBTEntry{};

            commandBuffer.bindPipeline(
                vk::PipelineBindPoint::eRayTracingKHR,
                pipeline);

            commandBuffer.bindDescriptorSets(
                vk::PipelineBindPoint::eRayTracingKHR,
                *pipelineLayout,
                0,
                {*descriptorManager->getDescriptorSet()},
                {});

            commandBuffer.traceRaysKHR(
                raygenShaderSBTEntry,
                missShaderSBTEntry,
                hitShaderSBTEntry,
                callableShaderSBTEntry,
                CUDA::RAYS_PER_PATCH, // ilosc promieni do wystrzelenia w poziomie - trzeba bedzie to dopasowac do ilosci promieni wystrzelonych w hemisferze
                CUDA::SELECTED_PATCHES_COUNT, // jedna warstwa dispatchu na kazdy selected patch
                1);

            if (visibilityOutputBuffer)
            {
                vk::BufferMemoryBarrier2 outputBufferBarrier{
                    .srcStageMask = vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
                    .srcAccessMask = vk::AccessFlagBits2::eShaderWrite,
                    .dstStageMask = vk::PipelineStageFlagBits2::eAllCommands,
                    .dstAccessMask = vk::AccessFlagBits2::eMemoryRead,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .buffer = visibilityOutputBuffer,
                    .offset = 0,
                    .size = VK_WHOLE_SIZE
                };

                vk::DependencyInfo dependencyInfo{
                    .memoryBarrierCount = 0,
                    .pMemoryBarriers = nullptr,
                    .bufferMemoryBarrierCount = 1,
                    .pBufferMemoryBarriers = &outputBufferBarrier,
                    .imageMemoryBarrierCount = 0,
                    .pImageMemoryBarriers = nullptr
                };

                commandBuffer.pipelineBarrier2(dependencyInfo);
            }

            commandBuffer.end();
        }
    }

}
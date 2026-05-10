#include "pch.h"
#include "RayTracingPipeline.hpp"

namespace VRTR
{
    RayTracingPipeline::RayTracingPipeline(RendererContext& ctx, PushConstant& pushConstantData)
        : ctx(ctx), pushConstantData(pushConstantData)
    {
    }

    void RayTracingPipeline::init(std::vector<vk::Image>& swapChainImages,
                                DescriptorResources& resources,
                                std::shared_ptr<CommandBufferManager>& commandBufferManager,
                                int width, int height
                                )
    {
        this->swapChainImages = &swapChainImages;
        this->commandBufferManager = commandBufferManager;
        this->width = width;
        this->height = height;

        storageImage = std::make_unique<StorageImage>(ctx, width, height);
        storageImage->init();

        // TODO meh
        resources.storageImageView = storageImage->getImageViewHandle();

        descriptorManager = std::make_unique<DescriptorManager>(ctx);
        descriptorManager->init(resources);

        createRayTracingPipeline();
        createShaderBindingTable();

        buildRTCommandBuffers();
    }

    void RayTracingPipeline::recreateStorageImage(int width, int height)
    {
        storageImage->recreate(width, height);
    }

    void RayTracingPipeline::updatePipelineDescriptors(DescriptorResources& resources, int width, int height)
    {
        this->width = width;
        this->height = height;

        resources.storageImageView = storageImage->getImageViewHandle();
        descriptorManager->setDescriptorResources(resources);

        descriptorManager->updateDescriptorSets();
        buildRTCommandBuffers();
    }

    void RayTracingPipeline::createRayTracingPipeline()
    {
        VRTR_DEBUG("Creating Ray Tracing Pipeline");

        const vk::PushConstantRange pushConstantRange{
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR,
            .offset = 0,
            .size = sizeof(PushConstant)};

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
            .flags = {},
            .setLayoutCount = 1,
            .pSetLayouts = &(*descriptorManager->getDescriptorSetLayout()),
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstantRange};

        rayTracingPipelineLayout = vk::raii::PipelineLayout(ctx.logicalDevice, pipelineLayoutInfo);

        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
        Shader shader;

        // Raygen shader
        shaderStages.push_back(shader.createShaderStageInfo(ctx.logicalDevice, CONSTANTS::SHADERS_DIR / "renderPass.spv", vk::ShaderStageFlagBits::eRaygenKHR, "rayGenShader"));
        vk::RayTracingShaderGroupCreateInfoKHR raygenGroup{
            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
            .generalShader = 0, // first entry in shaderStages
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR};
        shaderGroups.push_back(raygenGroup);

        // Miss shader
        shaderStages.push_back(shader.createShaderStageInfo(ctx.logicalDevice, CONSTANTS::SHADERS_DIR / "renderPass.spv", vk::ShaderStageFlagBits::eMissKHR, "missShader"));
        vk::RayTracingShaderGroupCreateInfoKHR missGroup{
            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
            .generalShader = 1, // second entry in shaderStages
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR};
        shaderGroups.push_back(missGroup);

        // Closest hit shader
        shaderStages.push_back(shader.createShaderStageInfo(ctx.logicalDevice, CONSTANTS::SHADERS_DIR / "renderPass.spv", vk::ShaderStageFlagBits::eClosestHitKHR, "closestHitShader"));
        vk::RayTracingShaderGroupCreateInfoKHR hitGroup{
            .type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
            .generalShader = VK_SHADER_UNUSED_KHR,
            .closestHitShader = 2, // third entry in shaderStages
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR};
        shaderGroups.push_back(hitGroup);

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
            .layout = rayTracingPipelineLayout,
            .basePipelineHandle = nullptr,
            .basePipelineIndex = -1};
        rayTracingPipeline = ctx.logicalDevice.createRayTracingPipelineKHR(nullptr, nullptr, pipelineInfo, nullptr);
    }


    void RayTracingPipeline::createShaderBindingTable()
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
        shader_handle_storage = rayTracingPipeline.getRayTracingShaderGroupHandlesKHR<uint8_t>(0, group_count, sbt_size);

        // KOPIOWANIE DANYCH Z CPU DO GPU:
        // najpierw mapujemy pamiec, zeby uzyskac wskaznik do pamieciu CPU z ktorego
        // Dane będą kopiowane do pamieci GPU
        // Potem kopiujemy dane do pamieci CPU
        // Na koniec odmapowujemy pamiec
        // Bawimy sie handle_size_aligned, zeby uzyskiwac konrketne bajty w pamieci CPU,
        // kazdy shader jest zapisany w odpowieniej, stałej odleglosci od poczatku pamieci CPU - handle_size_aligned
        // unmap - odmapowanie pamieci - explicit zakończenie kopiowania danych

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
    };

    void RayTracingPipeline::buildRTCommandBuffers()
    {
        VRTR_DEBUG("Building Ray Tracing Command Buffers");
        vk::CommandBufferBeginInfo beginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse,
            .pInheritanceInfo = nullptr};

        auto &commandBuffers = commandBufferManager->getCommandBuffers();

        if (commandBuffers.size() != swapChainImages->size()) {
            VRTR_CRITICAL("Mismatch between command buffer count ({}) and swapchain image count ({})",
                        commandBuffers.size(), swapChainImages->size());
        }

        for (uint32_t i = 0; i < commandBuffers.size(); i++)
        {
            auto &commandBuffer = commandBufferManager->getCommandBuffer(i);
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
                rayTracingPipeline);

            commandBuffer.bindDescriptorSets(
                vk::PipelineBindPoint::eRayTracingKHR,
                *rayTracingPipelineLayout,
                0,
                {*descriptorManager->getDescriptorSet()},
                {});

            std::array<uint64_t, 2> pushConstants = {{
                    pushConstantData.vertices,
                    pushConstantData.indices
                }};

            commandBuffer.pushConstants<uint64_t>(
                *rayTracingPipelineLayout,
                vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR,
                0,
                pushConstants
            );

            commandBufferManager->transition_image_layout(
                commandBuffer,
                *storageImage->getImage(),
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eGeneral,
                {},
                vk::AccessFlagBits2::eShaderWrite,
                vk::PipelineStageFlagBits2::eTopOfPipe,
                vk::PipelineStageFlagBits2::eRayTracingShaderKHR);

            commandBuffer.traceRaysKHR(
                raygenShaderSBTEntry,
                missShaderSBTEntry,
                hitShaderSBTEntry,
                callableShaderSBTEntry,
                width,
                height,
                1);

            commandBufferManager->transition_image_layout(
                commandBuffer,
                swapChainImages->at(i),
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal, // to jest potrzebne do kopiowania z storage image do swapchain image
                {},
                {},
                {},
                {});

            commandBufferManager->transition_image_layout(
                commandBuffer,
                *storageImage->getImage(),
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::eTransferSrcOptimal, // to jest potrzebne do kopiowania z storage image do swapchain image
                {},
                vk::AccessFlagBits2::eTransferRead,
                vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
                vk::PipelineStageFlagBits2::eTransfer);

            vk::ImageCopy copyRegion{
                .srcSubresource = vk::ImageSubresourceLayers{
                    vk::ImageAspectFlagBits::eColor,
                    0, 0, 1
                },
                .srcOffset = vk::Offset3D{0, 0, 0},
                .dstSubresource = vk::ImageSubresourceLayers{
                    vk::ImageAspectFlagBits::eColor,
                     0, 0, 1
                },
                .dstOffset = vk::Offset3D{0, 0, 0},
                .extent = vk::Extent3D{storageImage->getWidth(), storageImage->getHeight(), 1}};

            commandBuffer.copyImage(
                *storageImage->getImage(), vk::ImageLayout::eTransferSrcOptimal,
                swapChainImages->at(i), vk::ImageLayout::eTransferDstOptimal,
                {copyRegion});

            commandBufferManager->transition_image_layout(
                commandBuffer,
                swapChainImages->at(i),
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::ePresentSrcKHR,
                {},
                {},
                {},
                {});

            commandBuffer.end();
        }
    }

    void RayTracingPipeline::initRayTracing(RendererContext &ctx)
    {
        auto prop = ctx.gpu.getProperties2<vk::PhysicalDeviceProperties2,
                                    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR,
                                    vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();

        // rayTracingPipelineProperties = prop.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

        ctx.properties.rtPipelineProperties = prop.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
        ctx.properties.asProperties = prop.get<vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();
    }
}

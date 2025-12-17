#include "pch.h"
#include "RayTracingPipeline.hpp"

namespace VRTR
{
    RayTracingPipeline::RayTracingPipeline(VULKAN_CONTEXT& ctx)
        : ctx(ctx)
    {
    }

    void RayTracingPipeline::init(std::shared_ptr<DescriptorManager>& descriptorManager,
                                std::vector<vk::Image>& swapChainImages,
                                std::shared_ptr<CommandBufferManager>& commandBufferManager,
                                int width, int height,
                                std::shared_ptr<StorageImage>& storageImage
                                )
    {
        createRayTracingPipeline(descriptorManager);
        createShaderBindingTable();
    }

    void RayTracingPipeline::createRayTracingPipeline(std::shared_ptr<DescriptorManager>& descriptorManager)
    {
        VRTR_DEBUG("Creating Ray Tracing Pipeline");
        vk::DescriptorSetLayoutBinding ASLayout{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eAccelerationStructureKHR,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR,
            .pImmutableSamplers = nullptr};

        vk::DescriptorSetLayoutBinding storageImageLayout{
            .binding = 1,
            .descriptorType = vk::DescriptorType::eStorageImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR,
            .pImmutableSamplers = nullptr};

        vk::DescriptorSetLayoutBinding uniformBufferLayout{
            .binding = 2,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR,
            .pImmutableSamplers = nullptr};

        std::array<vk::DescriptorSetLayoutBinding, 3> bindings =
            {
                ASLayout,
                storageImageLayout,
                uniformBufferLayout
            };

        vk::DescriptorSetLayoutCreateInfo layoutInfo{
            .flags = {},
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()};
        auto descriptorSetLayout = vk::raii::DescriptorSetLayout(ctx.logicalDevice, layoutInfo);
        descriptorManager->setDescriptorSetLayout(descriptorSetLayout);

        // nei wiem co to xdd
        // const vk::PushConstantRange pushConstantRange{
        //     .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR,
        //     .offset = 0,
        //     .size = sizeof(PushConstantData)};

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
            .flags = {},
            .setLayoutCount = 1,
            .pSetLayouts = &(*descriptorManager->getDescriptorSetLayout()),
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr};
        rayTracingPipelineLayout = vk::raii::PipelineLayout(ctx.logicalDevice, pipelineLayoutInfo);

        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
        Shader shader;

        // Raygen shader
        shaderStages.push_back(shader.createShaderStageInfo(ctx.logicalDevice, CONSTANTS::SHADERS_DIR / "raygen.spv", vk::ShaderStageFlagBits::eRaygenKHR));
        vk::RayTracingShaderGroupCreateInfoKHR raygenGroup{
            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
            .generalShader = 0, // first entry in shaderStages
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR};
        shaderGroups.push_back(raygenGroup);

        // Miss shader
        shaderStages.push_back(shader.createShaderStageInfo(ctx.logicalDevice, CONSTANTS::SHADERS_DIR / "miss.spv", vk::ShaderStageFlagBits::eMissKHR));
        vk::RayTracingShaderGroupCreateInfoKHR missGroup{
            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
            .generalShader = 1, // second entry in shaderStages
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR};
        shaderGroups.push_back(missGroup);

        // Closest hit shader
        shaderStages.push_back(shader.createShaderStageInfo(ctx.logicalDevice, CONSTANTS::SHADERS_DIR / "closesthit.spv", vk::ShaderStageFlagBits::eClosestHitKHR));
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
            .maxPipelineRayRecursionDepth = 1,
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
        const uint32_t handle_size = ctx.rtPipelineProperties.shaderGroupHandleSize; // rozmiar jednego shader group
        const uint32_t handle_alignment = ctx.rtPipelineProperties.shaderGroupHandleAlignment;
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

    void RayTracingPipeline::buildRTCommandBuffers(std::vector<vk::Image>& swapChainImages,
                                         std::shared_ptr<CommandBufferManager>& commandBufferManager,
                                         std::shared_ptr<DescriptorManager>& descriptorManager,
                                         int width, int height,
                                         std::shared_ptr<StorageImage>& storageImage)
    {
        vk::CommandBufferBeginInfo beginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse,
            .pInheritanceInfo = nullptr};

        // vk::ImageSubresourceRange subresourceRange
        // {
        //     .aspectMask = vk::ImageAspectFlagBits::eColor,
        //     .baseMipLevel = 0,
        //     .levelCount = 1,
        //     .baseArrayLayer = 0,
        //     .layerCount = 1
        // };

        auto &commandBuffers = commandBufferManager->getCommandBuffers();

        if (commandBuffers.size() != swapChainImages.size()) {
            VRTR_CRITICAL("Mismatch between command buffer count ({}) and swapchain image count ({})",
                        commandBuffers.size(), swapChainImages.size());
        }

        for (uint32_t i = 0; i < commandBuffers.size(); i++)
        {
            commandBufferManager->beginCommandBuffer(i, beginInfo);
            
            const uint32_t handle_size = ctx.rtPipelineProperties.shaderGroupHandleSize;
            const uint32_t handle_alignment = ctx.rtPipelineProperties.shaderGroupHandleAlignment;
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

            commandBufferManager->getCommandBuffer(i).bindPipeline(
                vk::PipelineBindPoint::eRayTracingKHR,
                rayTracingPipeline);

            commandBufferManager->getCommandBuffer(i).bindDescriptorSets(
                vk::PipelineBindPoint::eRayTracingKHR,
                *rayTracingPipelineLayout,
                0,
                {*descriptorManager->getDescriptorSet()},
                {});

            commandBufferManager->transition_image_layout(
                commandBufferManager->getCommandBuffer(i),
                *storageImage->getImage(),
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eGeneral,
                {},
                vk::AccessFlagBits2::eShaderWrite,
                vk::PipelineStageFlagBits2::eTopOfPipe,
                vk::PipelineStageFlagBits2::eRayTracingShaderKHR);

            commandBufferManager->getCommandBuffer(i).traceRaysKHR(
                raygenShaderSBTEntry,
                missShaderSBTEntry,
                hitShaderSBTEntry,
                callableShaderSBTEntry,
                width,
                height,
                1);

            commandBufferManager->transition_image_layout(
                commandBufferManager->getCommandBuffer(i),
                swapChainImages.at(i),
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal, // to jest potrzebne do kopiowania z storage image do swapchain image
                {},
                {},
                {},
                {});

            commandBufferManager->transition_image_layout(
                commandBufferManager->getCommandBuffer(i),
                *storageImage->getImage(),
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::eTransferSrcOptimal, // to jest potrzebne do kopiowania z storage image do swapchain image
                {},
                vk::AccessFlagBits2::eTransferRead,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::PipelineStageFlagBits2::eTransfer);

            vk::ImageCopy copyRegion{
                .srcSubresource = vk::ImageSubresourceLayers{
                    vk::ImageAspectFlagBits::eColor,
                    0, 0, 1},
                .srcOffset = vk::Offset3D{0, 0, 0},
                .dstSubresource = vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                .dstOffset = vk::Offset3D{0, 0, 0},
                .extent = vk::Extent3D{storageImage->getWidth(), storageImage->getHeight(), 1}};

            commandBufferManager->getCommandBuffer(i).copyImage(
                *storageImage->getImage(), vk::ImageLayout::eTransferSrcOptimal,
                swapChainImages.at(i), vk::ImageLayout::eTransferDstOptimal,
                {copyRegion});

            commandBufferManager->transition_image_layout(
                commandBufferManager->getCommandBuffer(i),
                swapChainImages.at(i),
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::ePresentSrcKHR,
                {},
                {},
                {},
                {});

            commandBufferManager->endCommandBuffer(i);
        }
    }

    void RayTracingPipeline::initRayTracing(VULKAN_CONTEXT &ctx)
    {
        auto prop = ctx.gpu.getProperties2<vk::PhysicalDeviceProperties2,
                                    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR,
                                    vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();

        // rayTracingPipelineProperties = prop.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

        ctx.rtPipelineProperties = prop.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
        ctx.asProperties = prop.get<vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();
    }
}

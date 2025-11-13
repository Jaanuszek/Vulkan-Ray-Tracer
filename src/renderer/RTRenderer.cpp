#include "RTRenderer.hpp"
#include "pch.h"

namespace VRTR
{
    RTRenderer::~RTRenderer()
    {
        ctx.logicalDevice.waitIdle();
    }

    void RTRenderer::init(GLFWwindow *window)
    {
        VRTR_DEBUG("RTRENDERER INIT");

        glfwGetFramebufferSize(window, &width, &height);

        ctx.instance = InstanceManager::createInstance(ctx.context, ctx.debugMessenger);
        // TODO replace it with
        // rendererContext.instance = InstanceManager::createInstance(rendererContext.context, rendererContext.debugMessenger);

        VULKAN_HPP_DEFAULT_DISPATCHER.init(static_cast<vk::Instance>(*ctx.instance));

        DeviceProperties deviceProps = DeviceManager::initDevice(window, ctx.instance);
        ctx.gpu = std::move(deviceProps.physicalDevice);
        ctx.surface = std::move(deviceProps.surface);
        ctx.logicalDevice = std::move(deviceProps.logicalDevice);
        ctx.queue = std::move(deviceProps.graphicsQueue);
        ctx.graphics_queue_index = deviceProps.graphicsQueueFamilyIndex;
        // TODO replace VULKAN_CONTEXT with RendererContext
        // rendererContext.graphics_queue_index = deviceProps.graphicsQueueFamilyIndex;
        // rendererContext.gpu = std::move(deviceProps.physicalDevice);
        // rendererContext.surface = std::move(deviceProps.surface);
        // rendererContext.logicalDevice = std::move(deviceProps.logicalDevice);
        // rendererContext.queue = std::move(deviceProps.graphicsQueue);

        VULKAN_HPP_DEFAULT_DISPATCHER.init(static_cast<vk::Device>(*ctx.logicalDevice));

        initRayTracing();

        swapChainManager = std::make_unique<SwapChainManager>(ctx.logicalDevice, ctx.gpu, ctx.surface);
        swapChainManager->init(window);
        // TODO replace VULKAN_CONTEXT with RendererContext
        // swapChainManager = std::make_unique<SwapChainManager>(rendererContext.logicalDevice, rendererContext.gpu, rendererContext.surface);

        commandBufferManager = std::make_unique<CommandBufferManager>(ctx.logicalDevice, ctx.graphics_queue_index);
        // TODO replace VULKAN_CONTEXT with RendererContext
        // commandBuffermanager = std::make_unique<CommandBufferManager>(rendererContext.logical
        commandBufferManager->init();

        createSyncObjects();

        createStorageImage();

        createScene();

        createRayTracingPipeline();

        createShaderBindingTable();

        createDescriptorSets();

        buildRTCommandBuffers();
    }

    uint32_t RTRenderer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
    {
        for (uint32_t i = 0; i < ctx.gpu.getMemoryProperties().memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (ctx.gpu.getMemoryProperties().memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable memory type");
    }

    void RTRenderer::createSyncObjects()
    {
        VRTR_DEBUG("Creating Sync Objects");

        ctx.presentCompleteSemaphores.clear();
        ctx.renderCompleteSemaphores.clear();
        ctx.drawFences.clear();

        vk::SemaphoreCreateInfo semaphoreInfo{
            .pNext = nullptr,
            .flags = {}};

        vk::FenceCreateInfo fenceInfo{
            .pNext = nullptr,
            .flags = vk::FenceCreateFlagBits::eSignaled // so we don't wait forever the first time
        };
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            ctx.presentCompleteSemaphores.emplace_back(vk::raii::Semaphore(ctx.logicalDevice, semaphoreInfo));
            ctx.renderCompleteSemaphores.emplace_back(vk::raii::Semaphore(ctx.logicalDevice, semaphoreInfo));
            ctx.drawFences.emplace_back(vk::raii::Fence(ctx.logicalDevice, fenceInfo));
        }
    }

    void RTRenderer::updateUniformBuffer()
    {
        uniform_data.proj_inverse = glm::inverse(camera->matrices.perspective);
        uniform_data.view_inverse = glm::inverse(camera->matrices.view);
        uniform_buffer->Update(&uniform_data, sizeof(UniformData));
    }

    void RTRenderer::initRayTracing()
    {
        auto prop = ctx.gpu.getProperties2<vk::PhysicalDeviceProperties2,
                                           vk::PhysicalDeviceRayTracingPipelinePropertiesKHR,
                                           vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();

        rayTracingPipelineProperties = prop.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

        ctx.rtPipelineProperties = rayTracingPipelineProperties; // I am not a big fan of this, but Will think about it later
        ctx.asProperties = prop.get<vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();
    }

    void RTRenderer::createBLAS()
    {
        VRTR_DEBUG("Creating BLAS");

        std::vector<VertexRT> verticesRT = {
            {{1.0f, 1.0f, 0.0f}},
            {{-1.0f, 1.0f, 0.0f}},
            {{0.0f, -1.0f, 0.0f}}};
        std::vector<uint32_t> indicesRT = {0, 1, 2};

        size_t vertex_buffer_size = verticesRT.size() * sizeof(VertexRT);
        size_t index_buffer_size = indicesRT.size() * sizeof(uint32_t);

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
    }

    void RTRenderer::createTLAS()
    {
        VRTR_DEBUG("Creating TLAS");
        vk::TransformMatrixKHR transformMatrix{
            std::array<std::array<float, 4>, 3>{
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f}};

        vk::AccelerationStructureInstanceKHR ac_instance{
            .transform = transformMatrix,
            .instanceCustomIndex = 0,
            .mask = 0xFF,
            .instanceShaderBindingTableRecordOffset = 0,
            .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
            .accelerationStructureReference = blas_structure.device_address};

        std::unique_ptr<Buffer> instance_buffer = std::make_unique<Buffer>(ctx.logicalDevice,
                                                                           ctx.gpu,
                                                                           sizeof(vk::AccelerationStructureInstanceKHR),
                                                                           vk::BufferUsageFlags{},
                                                                           vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                                                           vk::BufferUsageFlagBits2::eShaderDeviceAddress | vk::BufferUsageFlagBits2::eAccelerationStructureBuildInputReadOnlyKHR);

        instance_buffer->Update(&ac_instance, sizeof(vk::AccelerationStructureInstanceKHR));

        vk::DeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
        instanceDataDeviceAddress.deviceAddress = instance_buffer->getDeviceAddress();

        vk::AccelerationStructureGeometryKHR ASGeometry{
            .pNext = nullptr,
            .geometryType = vk::GeometryTypeKHR::eInstances,
            .geometry = vk::AccelerationStructureGeometryInstancesDataKHR{
                .pNext = nullptr,
                .arrayOfPointers = VK_FALSE,
                .data = instanceDataDeviceAddress},
            .flags = vk::GeometryFlagBitsKHR::eOpaque};

        vk::AccelerationStructureBuildRangeInfoKHR ASBuildRangeInfo{
            .primitiveCount = 1, // jak dodam wiecej obiektow to to trzeba zamienic na ilosc instancji
            .primitiveOffset = 0,
            .firstVertex = 0,
            .transformOffset = 0};

        AS::createAccelerationStructure(ctx,
                                        vk::AccelerationStructureTypeKHR::eTopLevel,
                                        tlas_structure,
                                        ASGeometry,
                                        ASBuildRangeInfo,
                                        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
    }

    void RTRenderer::createScene()
    {
        VRTR_DEBUG("Creating scene");

        camera = std::make_unique<Camera>();
        camera->setPerspective(45.0f, static_cast<float>(width) / height, 0.1f, 100.0f);
        camera->setTranslation(glm::vec3(0.0f, 0.0f, -3.0f));
        camera->setRotation(glm::vec3(0.0f, 0.0f, 0.0f));

        uniform_buffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, sizeof(UniformData),
                                                  vk::BufferUsageFlagBits{},
                                                  vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                                  vk::BufferUsageFlagBits2::eUniformBuffer | vk::BufferUsageFlagBits2::eShaderDeviceAddress);

        updateUniformBuffer();

        createBLAS();
        createTLAS();
    }

    void RTRenderer::createStorageImage()
    {
        VRTR_DEBUG("Creating storage image");
        storageImage.width = static_cast<uint32_t>(width);
        storageImage.height = static_cast<uint32_t>(height);

        vk::ImageCreateInfo imgCreateInfo{
            .imageType = vk::ImageType::e2D,
            .format = vk::Format::eR8G8B8A8Unorm,
            .extent = vk::Extent3D{storageImage.width, storageImage.height, 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eOptimal,
            .usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc,
            .sharingMode = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined};
        storageImage.image = vk::raii::Image(ctx.logicalDevice, imgCreateInfo);

        vk::MemoryRequirements memRequirements = storageImage.image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)};
        storageImage.memory = vk::raii::DeviceMemory(ctx.logicalDevice, allocInfo);
        storageImage.image.bindMemory(*storageImage.memory, 0);

        vk::ImageViewCreateInfo viewCreateInfo{
            .image = *storageImage.image,
            .viewType = vk::ImageViewType::e2D,
            .format = vk::Format::eR8G8B8A8Unorm,
            .components = {
                vk::ComponentSwizzle::eIdentity, // it has to be identity inside storageImage
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity},
            .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
        storageImage.imageView = vk::raii::ImageView(ctx.logicalDevice, viewCreateInfo);

        // TODO OGARNAC TE TYMCZASOWE COMMAND BUFFERY
        vk::CommandBufferAllocateInfo cmdBufferAllocInfo{
            .commandPool = ctx.commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1};

        std::unique_ptr<TempCMDBufferManager> tempCmdBufferManager = std::make_unique<TempCMDBufferManager>(ctx.logicalDevice, ctx.queue, ctx.graphics_queue_index);
        vk::raii::CommandBuffer& tempCmdBuffer = tempCmdBufferManager->createTempCmdBuffer();

        CommandBufferManager::transition_image_layout(
            tempCmdBuffer,
            storageImage.image,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral, // it's basicaly storage image flag - we can do everything with it copy/write/read
            vk::AccessFlagBits2::eNone,
            vk::AccessFlagBits2::eShaderWrite,        // ?????????
            vk::PipelineStageFlagBits2::eAllCommands, // CHANGE IT LATER. It's very slow since GPU has to wait for all previous commands to finish
            vk::PipelineStageFlagBits2::eAllCommands  // CHANGE IT LATER
        );

        tempCmdBufferManager->submitAndWaitTempCmdBuffer();
    }

    void RTRenderer::createDescriptorSets()
    {
        VRTR_DEBUG("Creating Descriptor Sets");
        uint32_t maxSets = 1; // one for now, but later we will need more
        std::vector<vk::DescriptorPoolSize> poolSizes =
            {
                {vk::DescriptorType::eAccelerationStructureKHR, maxSets}, // wsparcie dla AS
                {vk::DescriptorType::eStorageImage, maxSets},             // umozliwienie zapisywania wyniku shaderow do storage image
                {vk::DescriptorType::eUniformBuffer, maxSets}             // wsparcie dla uniform bufferow (info ze sceny. np. macierz mvp)
            };

        // Descriptor Pool - zarządzanie pamiecią dla descriptor setów
        vk::DescriptorPoolCreateInfo poolInfo{
            .flags = {vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet},
            .maxSets = maxSets,
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes = poolSizes.data()};

        descriptorPool = vk::raii::DescriptorPool(ctx.logicalDevice, poolInfo);

        // Descriptor set - opis zasobów używanych przez shadery,
        // czyli layouty, bindingi ktore potem sie wykorzystujew shaderach
        vk::DescriptorSetAllocateInfo allocInfo{
            .descriptorPool = descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &*descriptorSetLayout};

        // A little workaround here, because Its not possible to create a single descriptor set in hpp vulkan
        // So I create descriptorSets (NOTE S on the end) and then move the first one to descriptorSet
        // https://github.com/KhronosGroup/Vulkan-Hpp/blob/938a2c36d2d3886a293c63c9a26417d6b0e2bc2d/vk_raii_ProgrammingGuide.md#09-create-a-vkraiidescriptorpool-and-vkraiidescriptorsets

        vk::raii::DescriptorSets tempDescriptorSets = vk::raii::DescriptorSets(ctx.logicalDevice, allocInfo);
        descriptorSet = std::move(tempDescriptorSets.front());

        vk::WriteDescriptorSetAccelerationStructureKHR descriptorASInfo{
            .pNext = nullptr,
            .accelerationStructureCount = 1,
            .pAccelerationStructures = &*tlas_structure.handle};

        vk::WriteDescriptorSet ASWrite{
            .pNext = &descriptorASInfo,
            .dstSet = *descriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eAccelerationStructureKHR,
        };

        vk::DescriptorImageInfo imageInfo{
            .sampler = {},
            .imageView = *storageImage.imageView,
            .imageLayout = vk::ImageLayout::eGeneral};

        vk::WriteDescriptorSet resultImageWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageImage,
            .pImageInfo = &imageInfo};

        vk::DescriptorBufferInfo bufferInfo{
            .buffer = uniform_buffer->getBuffer(),
            .offset = 0,
            .range = vk::WholeSize};

        vk::WriteDescriptorSet uniformBufferWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &bufferInfo};

        std::array<vk::WriteDescriptorSet, 3> WriteDescriptorSets = {
            ASWrite,
            resultImageWrite,
            uniformBufferWrite};
        ctx.logicalDevice.updateDescriptorSets(WriteDescriptorSets, {});
    }

    void RTRenderer::updateDescriptorSets()
    {
        vk::DescriptorImageInfo imageInfo{
            .sampler = {},
            .imageView = *storageImage.imageView,
            .imageLayout = vk::ImageLayout::eGeneral};

        vk::WriteDescriptorSet resultImageWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageImage,
            .pImageInfo = &imageInfo};
        std::array<vk::WriteDescriptorSet, 1> WriteDescriptorSets{resultImageWrite};
        ctx.logicalDevice.updateDescriptorSets(WriteDescriptorSets, {});
    }

    void RTRenderer::createRayTracingPipeline()
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
                uniformBufferLayout};

        vk::DescriptorSetLayoutCreateInfo layoutInfo{
            .flags = {},
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()};
        descriptorSetLayout = vk::raii::DescriptorSetLayout(ctx.logicalDevice, layoutInfo);

        // nei wiem co to xdd
        // const vk::PushConstantRange pushConstantRange{
        //     .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR,
        //     .offset = 0,
        //     .size = sizeof(PushConstantData)};

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
            .flags = {},
            .setLayoutCount = 1,
            .pSetLayouts = &*descriptorSetLayout,
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

    void RTRenderer::createShaderBindingTable()
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
    }

    void RTRenderer::buildRTCommandBuffers()
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


        //TODO tu mogą byc problemy, bo pobieram kopie, a nie referencje do obrazow swapchain
        // te komendy transition_image_layout nic nei robią z tymi obiektami (CHYBA)
        // ale warto miec to na uwadze
        auto swapChainImages = swapChainManager->getSwapChainImages();
        auto &commandBuffers = commandBufferManager->getCommandBuffers();

        if (commandBuffers.size() != swapChainImages.size()) {
            VRTR_CRITICAL("Mismatch between command buffer count ({}) and swapchain image count ({})",
                        commandBuffers.size(), swapChainImages.size());
        }

        for (uint32_t i = 0; i < commandBuffers.size(); i++)
        {
            commandBufferManager->beginCommandBuffer(i, beginInfo);

            const uint32_t handle_size = rayTracingPipelineProperties.shaderGroupHandleSize;
            const uint32_t handle_alignment = rayTracingPipelineProperties.shaderGroupHandleAlignment;
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
                {*descriptorSet},
                {});

            commandBufferManager->transition_image_layout(
                commandBufferManager->getCommandBuffer(i),
                *storageImage.image,
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
                *storageImage.image,
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
                .extent = vk::Extent3D{storageImage.width, storageImage.height, 1}};

            commandBufferManager->getCommandBuffer(i).copyImage(
                *storageImage.image, vk::ImageLayout::eTransferSrcOptimal,
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

    void RTRenderer::drawFrame(GLFWwindow *window)
    {
        vk::raii::SwapchainKHR &swapChain = swapChainManager->getSwapChain();
        while (vk::Result::eTimeout == ctx.logicalDevice.waitForFences(*ctx.drawFences.at(commandBufferManager->getCurrentFrame()), VK_TRUE, UINT64_MAX))
            ;

        // Unfortunately it needs to be inside try catch block, because "acquireNextImage" is throwing exceptions
        // I cant disable it, because i am using vk::raii and it requires exceptions to be enabled :(
        try
        {
            auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, ctx.presentCompleteSemaphores.at(commandBufferManager->getSemaphoreIndex()), nullptr);
            // recordCommandBuffer(imageIndex);
            ctx.logicalDevice.resetFences({ctx.drawFences[commandBufferManager->getCurrentFrame()]});
            vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eAllCommands);
            const vk::SubmitInfo submitInfo{
                .pNext = nullptr,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &*ctx.presentCompleteSemaphores.at(commandBufferManager->getSemaphoreIndex()),
                .pWaitDstStageMask = &waitDestinationStageMask,
                .commandBufferCount = 1,
                .pCommandBuffers = &*commandBufferManager->getCommandBuffer(imageIndex),
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = &*ctx.renderCompleteSemaphores.at(commandBufferManager->getCurrentFrame())};
            ctx.queue.submit({submitInfo}, *ctx.drawFences.at(commandBufferManager->getCurrentFrame()));

            const vk::PresentInfoKHR presentInfoKHR{
                .pNext = nullptr,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &*ctx.renderCompleteSemaphores.at(commandBufferManager->getCurrentFrame()),
                .swapchainCount = 1,
                .pSwapchains = &*swapChain,
                .pImageIndices = &imageIndex,
                .pResults = nullptr};

            result = ctx.queue.presentKHR(presentInfoKHR);

            commandBufferManager->setSemaphoreIndex((commandBufferManager->getSemaphoreIndex() + 1) % ctx.presentCompleteSemaphores.size());
            commandBufferManager->setCurrentFrame((commandBufferManager->getCurrentFrame() + 1) % MAX_FRAMES_IN_FLIGHT);
        }
        catch (const vk::OutOfDateKHRError &e)
        {
            swapChainManager->recreateSwapChain(window, width, height);
            createStorageImage();
            updateDescriptorSets();
            buildRTCommandBuffers();
            return;
        }
        catch (const std::exception &e)
        {
            VRTR_CRITICAL("Failed to acquire swap chain image!");
            throw std::runtime_error("Failed to acquire swap chain image!");
        }
    }
}
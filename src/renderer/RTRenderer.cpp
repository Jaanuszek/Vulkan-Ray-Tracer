#include "RTRenderer.hpp"
#include "pch.h"

namespace VRTR
{
    RTRenderer::RTRenderer(std::shared_ptr<Camera> camera) : camera(camera) {}

    RTRenderer::~RTRenderer()
    {
        ctx.logicalDevice.waitIdle();

        // Must explicitly destroy all VMA-managed resources BEFORE vmaDestroyAllocator.
        // Member destructors run AFTER the destructor body, so without this the
        // allocator would be destroyed while allocations are still live → VMA assert → abort().

        // Ptoblem jest taki, ze najpierw wywoluje sie destruktur RTRenderera,
        // a dopiero potem destruktory pól tej klasy
        models.clear();
        uniform_buffer.reset();
        geometrySBO.reset();
        materialSBO.reset();
        asManager.reset();

        vmaDestroyAllocator(vmaAlloc);
    }

    void RTRenderer::init(GLFWwindow *window)
    {
        VRTR_DEBUG("RTRENDERER INIT");
        modelInstanceOrder.clear();

        glfwGetFramebufferSize(window, &width, &height);

        ctx.instance = InstanceManager::createInstance(ctx);

        VULKAN_HPP_DEFAULT_DISPATCHER.init(static_cast<vk::Instance>(*ctx.instance));

        DeviceProperties deviceProps = DeviceManager::initDevice(window, ctx.instance);
        ctx.gpu = std::move(deviceProps.physicalDevice);
        ctx.surface = std::move(deviceProps.surface);
        ctx.logicalDevice = std::move(deviceProps.logicalDevice);
        ctx.queue = std::move(deviceProps.graphicsQueue);
        ctx.graphics_queue_index = deviceProps.graphicsQueueFamilyIndex;

        VULKAN_HPP_DEFAULT_DISPATCHER.init(static_cast<vk::Device>(*ctx.logicalDevice));

        setupVMA();

        RayTracingPipeline::initRayTracing(ctx);

        swapChainManager = std::make_unique<SwapChainManager>(ctx);
        swapChainManager->init(window);

        commandBufferManager = std::make_shared<CommandBufferManager>(ctx);
        commandBufferManager->init();

        createSyncObjects();

        storageImage = std::make_shared<StorageImage>(ctx, width, height);
        storageImage->init(commandBufferManager->getCommandPool());

        createScene();

        asManager = std::make_unique<AccelerationStructureManager>(ctx);

        // TODO dodać jakąś lepszą obsługę modeli
        // Uwzględnić to również w callbacku framebufferResize
        std::string viking_room_path = (CONSTANTS::ASSETS_DIR / "models/viking_room/").string();
        std::string viking_room_model_path = viking_room_path + "model/viking_room.obj";
        std::string viking_room_texture_path = viking_room_path + "textures/viking_room.png";

        uint32_t blasIndex = createModel(viking_room_model_path, viking_room_texture_path);

        std::string guy_model_path = (CONSTANTS::ASSETS_DIR / "models/guy/model/guy.obj").string();
        std::string guy_model_name = Model::getModelNameFromPath(guy_model_path);

        // uint32_t guyBlasIndex = createModel(guy_model_path, "");

        auto [floorVertices, floorIndices] = createFloor();
        Material floorMat{
            .albedo = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f),
            .type = MaterialType::METALLIC,
        };
        models.try_emplace("floor", std::make_unique<Model>(ctx, vmaAlloc, floorVertices, floorIndices, floorMat));
        uint32_t floorBlasIdx = asManager->createBLAS(models.at("floor"));

        glm::mat4 floorModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.1f, 0.0f));

        asManager->addInstance(floorBlasIdx, floorModel);
        modelInstanceOrder.push_back("floor");

        asManager->buildTLAS();

        // stworzenie storage buffora
        geometrySBO = std::make_unique<StorageBuffer>(ctx, vmaAlloc, sizeof(GeometryInfo));
        materialSBO = std::make_unique<StorageBuffer>(ctx, vmaAlloc, sizeof(Material));

        std::vector<GeometryInfo> geometryInfos;
        geometryInfos.reserve(modelInstanceOrder.size());
        std::vector<Material> materials;
        materials.reserve(modelInstanceOrder.size());
        for (const auto& modelName : modelInstanceOrder)
        {
            auto it = models.find(modelName);
            if (it == models.end())
            {
                throw std::runtime_error("Model missing for TLAS instance order: " + modelName);
            }
            auto& model = it->second;
            geometryInfos.push_back(model->getGeometryInfo());
            materials.push_back(model->getMaterial());
        }

        geometrySBO->copyDataToBuffer(geometryInfos.data(), geometryInfos.size() * sizeof(GeometryInfo));
        materialSBO->copyDataToBuffer(materials.data(), materials.size() * sizeof(Material));

        DescriptorResources descriptorResources{};
        descriptorResources.TLAS = asManager->getTLASHandle();
        descriptorResources.ubo = uniform_buffer->getBufferHandle();
        descriptorResources.storageImageView = storageImage->getImageViewHandle();
        descriptorResources.texImageView = models.at("viking_room")->getTexture().getTextureImageViewHandle();
        descriptorResources.texSampler = models.at("viking_room")->getTexture().getTextureSamplerHandle();
        descriptorResources.geometryInfoBuffer = geometrySBO->getBufferHandle();
        descriptorResources.materialBuffer = materialSBO->getBufferHandle();

        auto gi = models.at("viking_room")->getGeometryInfo();
        PushConstant vikingRoomModelPC{
            .vertices = gi.vertexBufferAddr,
            .indices = gi.indexBufferAddr
        };

        rayTracingPipeline = std::make_unique<RayTracingPipeline>(ctx, vikingRoomModelPC);
        rayTracingPipeline->init(swapChainManager->getSwapChainImages(),
                                descriptorResources,
                                commandBufferManager,
                                width,
                                height,
                                storageImage
                                );
    }

    void RTRenderer::setupVMA()
    {
        VmaVulkanFunctions vulkanFunctions{
            .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
            .vkCreateBuffer = vkCreateBuffer,
            .vkCreateImage = vkCreateImage,
        };

        VmaAllocatorCreateInfo allocatorCI{
            .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
            .physicalDevice = *ctx.gpu,
            .device = *ctx.logicalDevice,
            .pVulkanFunctions = &vulkanFunctions,
            .instance = *ctx.instance,
            .vulkanApiVersion = VK_API_VERSION_1_4,
        };

        vmaCreateAllocator(&allocatorCI, &vmaAlloc);
    }

    void RTRenderer::createSyncObjects()
    {
        VRTR_DEBUG("Creating Sync Objects");

        presentCompleteSemaphores.clear();
        renderCompleteSemaphores.clear();
        drawFences.clear();

        vk::SemaphoreCreateInfo semaphoreInfo{
            .pNext = nullptr,
            .flags = {}};

        vk::FenceCreateInfo fenceInfo{
            .pNext = nullptr,
            .flags = vk::FenceCreateFlagBits::eSignaled // so we don't wait forever the first time
        };
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            presentCompleteSemaphores.emplace_back(vk::raii::Semaphore(ctx.logicalDevice, semaphoreInfo));
            renderCompleteSemaphores.emplace_back(vk::raii::Semaphore(ctx.logicalDevice, semaphoreInfo));
            drawFences.emplace_back(vk::raii::Fence(ctx.logicalDevice, fenceInfo));
        }
    }

    uint32_t RTRenderer::createModel(std::string modelPath, std::string texturePath)
    {
        Material mat{
            .albedo = glm::vec4(0.1f,0.4f, 0.8f, 1.0f),
            // .type = MaterialType::METALLIC,
        };

        std::string model_name = Model::getModelNameFromPath(modelPath);
        auto [it, inserted] = models.try_emplace(
            model_name,
            std::make_unique<Model>(
            ctx,
            vmaAlloc,
            modelPath,
            texturePath,
            mat));

        uint32_t blasIndex = asManager->createBLAS(models.at(model_name));

        glm::mat4 rotatedModel = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        asManager->addInstance(blasIndex, rotatedModel);
        modelInstanceOrder.push_back(model_name);
        return blasIndex;
    }

    void RTRenderer::updateUniformBuffer()
    {
        uniform_data.proj_inverse = glm::inverse(camera->matrices.perspective);
        uniform_data.view_inverse = glm::inverse(camera->matrices.view);
        uniform_buffer->Update(&uniform_data, sizeof(UniformData));
    }

    void RTRenderer::createScene()
    {
        VRTR_DEBUG("Creating scene");

        uniform_buffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, sizeof(UniformData),
                                                  vk::BufferUsageFlagBits{},
                                                  vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                                  vk::BufferUsageFlagBits2::eUniformBuffer | vk::BufferUsageFlagBits2::eShaderDeviceAddress);

        updateUniformBuffer();
    }

    void RTRenderer::recreateResources(GLFWwindow *window)
    {
        swapChainManager->recreateSwapChain(window, width, height);
        storageImage->recreate(commandBufferManager->getCommandPool(), width, height);

        // Update camera perspective with new aspect ratio
        camera->setPerspective(45.0f, static_cast<float>(width) / height, 0.1f, 100.0f);

        DescriptorResources desResources{};
        desResources.TLAS = asManager->getTLASHandle();
        desResources.ubo = uniform_buffer->getBufferHandle();
        desResources.storageImageView = storageImage->getImageViewHandle();
        // temporary solution
        for(auto& [name, model] : models)
        {
            if (model->hasTexture())
            {
                desResources.texImageView = model->getTexture().getTextureImageViewHandle();
                desResources.texSampler = model->getTexture().getTextureSamplerHandle();
                break;
            }
        }
        desResources.geometryInfoBuffer = geometrySBO->getBufferHandle();
        desResources.materialBuffer = materialSBO->getBufferHandle();

        rayTracingPipeline->updatePipelineDescriptors(desResources, width, height);   
    }

    std::pair<std::vector<VertexRT>, std::vector<uint32_t>> RTRenderer::createFloor()
    {
        std::vector<VertexRT> vertices = {
            {{-5.0f, 0.0f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
            {{5.0f, 0.0f, -5.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
            {{5.0f, 0.0f, 5.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
            {{-5.0f, 0.0f, 5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}
        };

        std::vector<uint32_t> indices = {
            0, 1, 2,
            2, 3, 0
        };

        return {vertices, indices};
    }

    glm::mat4 RTRenderer::rotateModel(float angle, const glm::vec3 &axis)
    {
        return glm::rotate(glm::mat4(1.0f), angle, axis);
    }

    void RTRenderer::drawFrame(GLFWwindow *window, double deltaTime)
    {
        vk::raii::SwapchainKHR &swapChain = swapChainManager->getSwapChain();
        while (vk::Result::eTimeout == ctx.logicalDevice.waitForFences(*drawFences.at(commandBufferManager->getCurrentFrame()), VK_TRUE, UINT64_MAX))
            ;

        // Unfortunately it needs to be inside try catch block, because "acquireNextImage" is throwing exceptions
        // I cant disable it, because i am using vk::raii and it requires exceptions to be enabled :(
        try
        {
            auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, presentCompleteSemaphores.at(commandBufferManager->getSemaphoreIndex()), nullptr);
            ctx.logicalDevice.resetFences({drawFences[commandBufferManager->getCurrentFrame()]});
            vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eAllCommands);

            asManager->updateTLAS(deltaTime);
            updateUniformBuffer(); // Camera UBO update
            const vk::SubmitInfo submitInfo{
                .pNext = nullptr,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &*presentCompleteSemaphores.at(commandBufferManager->getSemaphoreIndex()),
                .pWaitDstStageMask = &waitDestinationStageMask,
                .commandBufferCount = 1,
                .pCommandBuffers = &*commandBufferManager->getCommandBuffer(imageIndex),
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = &*renderCompleteSemaphores.at(commandBufferManager->getCurrentFrame())};
            ctx.queue.submit({submitInfo}, *drawFences.at(commandBufferManager->getCurrentFrame()));

            const vk::PresentInfoKHR presentInfoKHR{
                .pNext = nullptr,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &*renderCompleteSemaphores.at(commandBufferManager->getCurrentFrame()),
                .swapchainCount = 1,
                .pSwapchains = &*swapChain,
                .pImageIndices = &imageIndex,
                .pResults = nullptr};

            result = ctx.queue.presentKHR(presentInfoKHR);

            commandBufferManager->setSemaphoreIndex((commandBufferManager->getSemaphoreIndex() + 1) % presentCompleteSemaphores.size());
            commandBufferManager->setCurrentFrame((commandBufferManager->getCurrentFrame() + 1) % MAX_FRAMES_IN_FLIGHT);
        }
        catch (const vk::OutOfDateKHRError &e)
        {
            recreateResources(window);
            return;
        }
        catch (const std::exception &e)
        {
            VRTR_CRITICAL("Failed to acquire swap chain image!");
            throw std::runtime_error("Failed to acquire swap chain image!");
        }
    }
}

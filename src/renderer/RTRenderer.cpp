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
        models.clear();
        uniform_buffer.reset();
        asManager.reset();

        vmaDestroyAllocator(vmaAlloc);
    }

    void RTRenderer::init(GLFWwindow *window)
    {
        VRTR_DEBUG("RTRENDERER INIT");

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

        asManager->buildTLAS();

        // stworzenie storage buffora

        vk::DeviceSize storageBufferSize = sizeof(GeometryInfo) * CONSTANTS::MAX_OBJECTS;

        vk::BufferCreateInfo storageBufferCI{
            .size = storageBufferSize,
            .usage = vk::BufferUsageFlagBits::eStorageBuffer,
            .sharingMode = vk::SharingMode::eExclusive,
        };

        VmaAllocationCreateInfo storageBufferAllocCI{
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        };
        VkBuffer rawStorageBuffer;
        VmaAllocation storageBufferAlloc;
        vmaCreateBuffer(vmaAlloc, reinterpret_cast<VkBufferCreateInfo*>(&storageBufferCI), &storageBufferAllocCI, &rawStorageBuffer, &storageBufferAlloc, nullptr);

        geometry_info_buffer = vk::raii::Buffer(ctx.logicalDevice, rawStorageBuffer);

        std::vector<GeometryInfo> geometryInfos;
        for (const auto& [name, model] : models)
        {
            auto info = model->getGeometryInfo();
            geometryInfos.push_back(info);
        }

        vmaCopyMemoryToAllocation(vmaAlloc, geometryInfos.data(), storageBufferAlloc, 0, geometryInfos.size() * sizeof(GeometryInfo));

        DescriptorResources descriptorResources{};
        descriptorResources.TLAS = &asManager->getTLAS();
        descriptorResources.ubo = &uniform_buffer->getBuffer();
        descriptorResources.storageImageView = &storageImage->getImageView();
        descriptorResources.texImageView = models.at("viking_room")->getTexture().getTextureImageView();
        descriptorResources.texSampler = models.at("viking_room")->getTexture().getTextureSampler();
        descriptorResources.geometryInfoBuffer = &geometry_info_buffer;
        // descriptorResources.materialBuffer = &it->second.getMaterialBuffer()->getBuffer();
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
        std::string model_name = Model::getModelNameFromPath(modelPath);
        auto [it, inserted] = models.try_emplace(
            model_name,
            std::make_unique<Model>(
            ctx,
            vmaAlloc,
            modelPath,
            texturePath));

        uint32_t blasIndex = asManager->createBLAS(models.at(model_name));

        glm::mat4 rotatedModel = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        asManager->addInstance(blasIndex, rotatedModel);
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
            swapChainManager->recreateSwapChain(window, width, height);
            storageImage->recreate(commandBufferManager->getCommandPool(), width, height);

            // Update camera perspective with new aspect ratio
            camera->setPerspective(45.0f, static_cast<float>(width) / height, 0.1f, 100.0f);

            DescriptorResources desResources{};
            desResources.TLAS = &asManager->getTLAS();
            desResources.ubo = &uniform_buffer->getBuffer();
            desResources.storageImageView = &storageImage->getImageView();
            // temporary solution
            for(auto& [name, model] : models)
            {
                if (model->hasTexture())
                {
                    desResources.texImageView = model->getTexture().getTextureImageView();
                    desResources.texSampler = model->getTexture().getTextureSampler();
                    break;
                }
            }

            rayTracingPipeline->updatePipelineDescriptors(desResources, width, height);
            return;
        }
        catch (const std::exception &e)
        {
            VRTR_CRITICAL("Failed to acquire swap chain image!");
            throw std::runtime_error("Failed to acquire swap chain image!");
        }
    }
}

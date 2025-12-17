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

        RayTracingPipeline::initRayTracing(ctx);

        swapChainManager = std::make_unique<SwapChainManager>(ctx.logicalDevice, ctx.gpu, ctx.surface);
        swapChainManager->init(window);
        // TODO replace VULKAN_CONTEXT with RendererContext
        // swapChainManager = std::make_unique<SwapChainManager>(rendererContext.logicalDevice, rendererContext.gpu, rendererContext.surface);

        commandBufferManager = std::make_shared<CommandBufferManager>(ctx.logicalDevice, ctx.graphics_queue_index);
        // TODO replace VULKAN_CONTEXT with RendererContext
        // commandBuffermanager = std::make_unique<CommandBufferManager>(rendererContext.logical
        commandBufferManager->init();

        createSyncObjects();

        storageImage = std::make_shared<StorageImage>(ctx.logicalDevice, ctx.gpu, ctx.commandPool, ctx.queue, ctx.graphics_queue_index, width, height);
        storageImage->init();

        createScene();

        asManager = std::make_unique<AccelerationStructureManager>(ctx);

        std::vector<VertexRT> verticesRT = {
            {{1.0f, 1.0f, 0.0f}},
            {{-1.0f, 1.0f, 0.0f}},
            {{0.0f, -1.0f, 0.0f}}};
        std::vector<uint32_t> indicesRT = {0, 1, 2};

        uint32_t blasIndex = asManager->createBLAS(verticesRT, indicesRT);

        asManager->addInstance(blasIndex, glm::mat4(1.0f));

        std::vector<VertexRT> floorVertices = {
            {{-5.0f, -1.0f, -5.0f}},
            {{5.0f, -1.0f, -5.0f}},
            {{5.0f, -1.0f, 5.0f}},
            {{-5.0f, -1.0f, 5.0f}}};
        std::vector<uint32_t> floorIndices = {0, 1, 2,
                                             2, 3, 0};
        uint32_t floorBlaIndex = asManager->createBLAS(floorVertices, floorIndices);
        asManager->addInstance(floorBlaIndex, glm::mat4(-1.0f));

        asManager->buildTLAS();

        descriptorManager = std::make_shared<DescriptorManager>(ctx);

        rayTracingPipeline = std::make_unique<RayTracingPipeline>(ctx);
        rayTracingPipeline->init(descriptorManager,
                                    swapChainManager->getSwapChainImages(),
                                    commandBufferManager,
                                    width,
                                    height,
                                    storageImage);

        // it has to be init after rayTracingPipeline is created because it uses its descriptor set layout
        descriptorManager->init(asManager->getTLAS(), uniform_buffer->getBuffer(), storageImage->getImageView());
        rayTracingPipeline->buildRTCommandBuffers(swapChainManager->getSwapChainImages(),
                                            commandBufferManager,
                                            descriptorManager,
                                            width,
                                            height,
                                            storageImage);
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

    void RTRenderer::createScene()
    {
        VRTR_DEBUG("Creating scene");

        camera = std::make_unique<Camera>();
        camera->setPerspective(45.0f, static_cast<float>(width) / height, 0.1f, 100.0f);
        camera->setTranslation(glm::vec3(0.0f, 0.0f, -7.0f));
        camera->setRotation(glm::vec3(0.0f, 0.0f, 0.0f));

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
            // camera->setRotation(glm::vec3(0.0f, 0.0f, static_cast<float>(16.0 * glfwGetTime())));
            // asManager->updateTLAS(rotateModel(static_cast<float>(2.0 * glfwGetTime()), glm::vec3(0.0f, 0.0f, 1.0f)));
            // updateUniformBuffer();
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
            storageImage->recreate(width, height);
            // tu obowiazkowo trzeba zrobic wrappery
            descriptorManager->updateDescriptorSets(storageImage->getImageView());
            rayTracingPipeline->buildRTCommandBuffers(swapChainManager->getSwapChainImages(), commandBufferManager, descriptorManager, width, height, storageImage);
            return;
        }
        catch (const std::exception &e)
        {
            VRTR_CRITICAL("Failed to acquire swap chain image!");
            throw std::runtime_error("Failed to acquire swap chain image!");
        }
    }
}
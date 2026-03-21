#include "RTRenderer.hpp"
#include "pch.h"

namespace VRTR
{
    RTRenderer::RTRenderer(std::shared_ptr<Camera> camera, SceneSettings &sceneSettings) 
    : camera(camera), sceneSettings(sceneSettings) {}

    RTRenderer::~RTRenderer()
    {
        ctx.logicalDevice.waitIdle();

        scene.reset();
        uniform_buffer.reset();
        gui.reset();

        vmaDestroyAllocator(ctx.vmaAllocator);
    }

    void RTRenderer::init(GLFWwindow *window)
    {
        VRTR_DEBUG("RTRENDERER INIT");

        glfwGetFramebufferSize(window, &width, &height);

        InstanceManager::createInstance(ctx);

        VULKAN_HPP_DEFAULT_DISPATCHER.init(static_cast<vk::Instance>(*ctx.instance));

        DeviceManager::initDevice(window, ctx);

        setupVMA();

        RayTracingPipeline::initRayTracing(ctx);

        swapChainManager = std::make_shared<SwapChainManager>(ctx);
        swapChainManager->init(window);

        commandBufferManager = std::make_shared<CommandBufferManager>(ctx);
        commandBufferManager->init();

        initImGUI(window);

        scene = std::make_unique<Scene>(ctx);
        scene->createScene(sceneSettings.ubo.light_pos);

        frameManager = std::make_unique<FrameManager>(ctx, swapChainManager);
        frameManager->init(scene->getPatches());

        uniform_buffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, sizeof(UniformData),
                                            vk::BufferUsageFlagBits{},
                                            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                            vk::BufferUsageFlagBits2::eUniformBuffer | vk::BufferUsageFlagBits2::eShaderDeviceAddress);

        updateUniformBuffer();

        DescriptorResources dr = buildDescriptorResources();

        auto gi = scene->getGeometryInfo("viking_room");
        PushConstant vikingRoomModelPC{
            .vertices = gi.vertexBufferAddr,
            .indices = gi.indexBufferAddr
        };

        rayTracingPipeline = std::make_unique<RayTracingPipeline>(ctx, vikingRoomModelPC);
        rayTracingPipeline->init(swapChainManager->getSwapChainImages(),
                                dr,
                                commandBufferManager,
                                width,
                                height
                                );

        visibilityPipeline = std::make_unique<VisibilityPipeline>(ctx);
        visibilityPipeline->init(dr, commandBufferManager);
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

        vmaCreateAllocator(&allocatorCI, &ctx.vmaAllocator);
    }

    void RTRenderer::initImGUI(GLFWwindow* window)
    {
        gui = std::make_unique<GUI>(ctx, sceneSettings);
        gui->init(window, width, height);
        gui->initResources(
            commandBufferManager->getCommandPool(),
            swapChainManager->getImageFormat(),
            static_cast<uint32_t>(swapChainManager->getSwapChainImages().size())
        );
    }

    void RTRenderer::updateUniformBuffer()
    {
        sceneSettings.ubo.proj_inverse = glm::inverse(camera->matrices.perspective);
        sceneSettings.ubo.view_inverse = glm::inverse(camera->matrices.view);
        sceneSettings.ubo.light_pos = sceneSettings.ubo.light_pos;
        uniform_buffer->Update(&sceneSettings.ubo, sizeof(UniformData));
    }

    DescriptorResources RTRenderer::buildDescriptorResources()
    {
        DescriptorResources dr{};
        dr.ubo = uniform_buffer->getBufferHandle();
        scene->appendDescriptorResources(dr);
        frameManager->appendDescriptorResources(dr);

        return dr;
    }

    void RTRenderer::recreateResources(GLFWwindow *window)
    {
        swapChainManager->recreateSwapChain(window, width, height);
        rayTracingPipeline->recreateStorageImage(width, height);

        // Update camera perspective with new aspect ratio
        camera->setPerspective(45.0f, static_cast<float>(width) / height, 0.1f, 100.0f);

        DescriptorResources dr = buildDescriptorResources();

        rayTracingPipeline->updatePipelineDescriptors(dr, width, height);   
    }

    void RTRenderer::drawFrame(GLFWwindow *window, double deltaTime, bool renderGUI)
    {
        try
        {
            uint32_t imageIndex = frameManager->acquireNextImage();

            frameManager->runCudaSelectPass(static_cast<uint32_t>(scene->getPatches().size()));

            frameManager->submitVisibilityQueue({*commandBufferManager->getVisibilityCommandBuffer(imageIndex)});

            frameManager->runCudaPostVisibilityPass();

            // Tu są wykonywane jakieś polecenia CPU, które nie są asynchroniczne
            std::vector<vk::CommandBuffer> submitCommandBuffers = {*commandBufferManager->getCommandBuffer(imageIndex)};
            if (renderGUI){
                gui->newFrame();

                vk::CommandBuffer guiCommandBuffer = gui->buildDrawCommandBuffer(
                    imageIndex,
                    swapChainManager->getSwapChainImage(imageIndex),
                    swapChainManager->getSwapChainImageView(imageIndex),
                    swapChainManager->getExtent());

                if(guiCommandBuffer != VK_NULL_HANDLE)
                {
                    submitCommandBuffers.push_back(guiCommandBuffer);
                }
            }

            if(gui->updateRequired())
            {
                switch (sceneSettings.transformations.updateRequest)
                {
                case UpdateRequest::Rotation:
                {
                    scene->updateTLAS(sceneSettings.transformations.rotationAngle);
                    break;
                }
                case UpdateRequest::LightPos:
                {   
                    uint32_t lightInstanceIdx = scene->getLightTLASIdx();
                    scene->updateInstanceTLAS(lightInstanceIdx, glm::translate(glm::mat4(1.0f), sceneSettings.ubo.light_pos));
                    break;
                }
                case UpdateRequest::PatchTriangleSize:
                {   scene->updatePatchData(static_cast<uint8_t>(sceneSettings.transformations.patchTriangleSize));
                    break;
                }
                default:
                    break;
                }

                gui->setUpdateNeed(false);
            }
            updateUniformBuffer(); // Camera UBO update

            frameManager->submitRenderQueue(submitCommandBuffers);
            // frameManager->runCudaFrame(static_cast<uint32_t>(scene->getPatches().size()));
            frameManager->presentFrame(imageIndex);
            frameCount++;
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

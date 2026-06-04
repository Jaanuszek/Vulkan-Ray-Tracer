#include "RTRenderer.hpp"
#include "pch.h"

#include <chrono>
#include <fstream>

namespace
{
    constexpr const char* RADIOSITY_TIMING_CSV = "radiosity_timing.csv";

    void resetRadiosityTimingCsv()
    {
        std::ofstream file(RADIOSITY_TIMING_CSV, std::ios::trunc);
        if (!file.is_open())
        {
            return;
        }

        file << "record_type,iteration,iteration_time_ms,average_time_ms,total_time_ms\n";
    }

    void appendRadiosityTimingCsv(const char* recordType,
                                  uint32_t iteration,
                                  double iterationTimeMs,
                                  double averageTimeMs,
                                  double totalTimeMs)
    {
        std::ofstream file(RADIOSITY_TIMING_CSV, std::ios::app);
        if (!file.is_open())
        {
            return;
        }

        file << recordType << ','
             << iteration << ','
             << iterationTimeMs << ','
             << averageTimeMs << ','
             << totalTimeMs << '\n';
    }
}

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

    void RTRenderer::init(GLFWwindow *window, bool buildResources)
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

        scene = std::make_unique<Scene>(ctx);

        if(buildResources)
        {
            initImGUI(window);

            // scene->createScene(sceneSettings.ubo.light_pos);

            frameManager = std::make_unique<FrameManager>(ctx, swapChainManager);
            frameManager->init(
                scene->getPatches(),
                scene->getVertexCount(),
                scene->getVertexPatchIndices(),
                scene->getVertexPatchOffsets()
            );

            uniform_buffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, sizeof(UniformData),
                                                vk::BufferUsageFlagBits{},
                                                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                                vk::BufferUsageFlagBits2::eUniformBuffer | vk::BufferUsageFlagBits2::eShaderDeviceAddress);

            updateUniformBuffer();

            DescriptorResources dr = buildDescriptorResources();

            auto gi = scene->getFirstGeometryInfo();
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
        else
        {
            VRTR_INFO("RTRenderer initialized without resource creation. Call buildResources() to set it up.");
        }
    }

    void RTRenderer::buildResources(GLFWwindow *window)
    {
        initImGUI(window);

        frameManager = std::make_unique<FrameManager>(ctx, swapChainManager);
        frameManager->init(
            scene->getPatches(),
            scene->getVertexCount(),
            scene->getVertexPatchIndices(),
            scene->getVertexPatchOffsets()
        );

        uniform_buffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, sizeof(UniformData),
                                            vk::BufferUsageFlagBits{},
                                            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                            vk::BufferUsageFlagBits2::eUniformBuffer | vk::BufferUsageFlagBits2::eShaderDeviceAddress);

        updateUniformBuffer();

        DescriptorResources dr = buildDescriptorResources();

        auto gi = scene->getFirstGeometryInfo();
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
        camera->setPerspective(60.0f, static_cast<float>(width) / height, 0.1f, 100.0f);

        DescriptorResources dr = buildDescriptorResources();

        rayTracingPipeline->updatePipelineDescriptors(dr, width, height);   
    }

    void RTRenderer::drawFrame(GLFWwindow *window, double deltaTime, bool renderGUI)
    {
        try
        {
            uint32_t imageIndex = frameManager->acquireNextImage();

            if(frameManager->isComputeRadiosity())
            {
                const auto radiosityIterationStart = std::chrono::steady_clock::now();

                frameManager->runCudaSelectPass(static_cast<uint32_t>(scene->getPatches().size()));

                frameManager->submitVisibilityQueue({*commandBufferManager->getVisibilityCommandBuffer(imageIndex)});

                frameManager->runCudaPostVisibilityPass(scene->getPatches().size(), scene->getVertexCount());

                const auto radiosityIterationEnd = std::chrono::steady_clock::now();
                const double radiosityIterationMs = std::chrono::duration<double, std::milli>(radiosityIterationEnd - radiosityIterationStart).count();

                ++radiosityIterationCount;
                radiosityIterationTimeMsTotal += radiosityIterationMs;

                VRTR_DEBUG("Radiosity iteration {} took {} ms", radiosityIterationCount, radiosityIterationMs);
                appendRadiosityTimingCsv(
                    "iteration",
                    radiosityIterationCount,
                    radiosityIterationMs,
                    radiosityIterationTimeMsTotal / static_cast<double>(radiosityIterationCount),
                    radiosityIterationTimeMsTotal
                );

                if(frameManager->getCOVERAGED())
                {
                    const double averageRadiosityFrameTimeMs = radiosityIterationTimeMsTotal / static_cast<double>(radiosityIterationCount);
                    VRTR_DEBUG("Radiosity coveraged after {} iterations", radiosityIterationCount);
                    VRTR_DEBUG("Average radiosity frame time: {} ms", averageRadiosityFrameTimeMs);
                    appendRadiosityTimingCsv(
                        "summary",
                        radiosityIterationCount,
                        radiosityIterationMs,
                        averageRadiosityFrameTimeMs,
                        radiosityIterationTimeMsTotal
                    );
                    frameManager->setComputeRadiosity(false);
                    sceneSettings.ubo.enableRadiosityPass = false;
                    radiosityBootstrapDone = true;
                }

                // ++radiosityDemoFrameCounter;
                // if (radiosityDemoFrameCounter >= 1)
                // {
                //     frameManager->setComputeRadiosity(false);
                //     sceneSettings.ubo.enableRadiosityPass = false;
                //     sceneSettings.ubo.useRadiosityLightmap = true;
                //     radiosityBootstrapDone = true;
                // }
            }

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
                case UpdateRequest::EnableRadiosityPass:
                {
                    frameManager->setComputeRadiosity(sceneSettings.ubo.enableRadiosityPass);
                    if (sceneSettings.ubo.enableRadiosityPass)
                    {
                        radiosityIterationCount = 0;
                        radiosityIterationTimeMsTotal = 0.0;
                        resetRadiosityTimingCsv();
                        VRTR_DEBUG("Radiosity timing CSV reset: {}", RADIOSITY_TIMING_CSV);
                    }
                    // if (sceneSettings.ubo.enableRadiosityPass)
                    // {
                    //     radiosityDemoFrameCounter = 0;
                    //     radiosityBootstrapDone = false;
                    // }
                    break;
                }
                default:
                    break;
                }

                gui->setUpdateNeed(false);
            }
            updateUniformBuffer(); // Camera UBO update

            frameManager->submitRenderQueue(submitCommandBuffers);
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

    // High-level wrappers
    uint32_t RTRenderer::addModel(const std::string& modelPath, const std::string& texPath, const glm::mat4& transform)
    {
        if(!scene)
            throw std::runtime_error("Scene not initialized");
        return scene->importModel(modelPath, texPath, transform);
    }

    uint32_t RTRenderer::addMesh(const std::string& name, const std::vector<VertexRT>& vertices, const std::vector<uint32_t>& indices, const std::vector<Material>& mats, const glm::mat4& transform)
    {
        if(!scene)
            throw std::runtime_error("Scene not initialized");
        return scene->addObject(name, vertices, indices, mats, transform);
    }

    void RTRenderer::buildTLAS()
    {
        if(!scene)
            throw std::runtime_error("Scene not initialized");
        scene->buildTLAS();
    }

    void RTRenderer::setInstanceTransform(uint32_t instanceIdx, const glm::mat4& newTransform)
    {
        if(!scene)
            throw std::runtime_error("Scene not initialized");
        scene->updateInstanceTLAS(instanceIdx, newTransform);
    }

    void RTRenderer::rotateScene(float rotationAngle)
    {
        if(!scene)
            throw std::runtime_error("Scene not initialized");
        scene->updateTLAS(rotationAngle);
    }

    void RTRenderer::setLightPosition(const glm::vec3& pos)
    {
        if(!scene)
            throw std::runtime_error("Scene not initialized");
        uint32_t lightIdx = scene->getLightTLASIdx();
        scene->updateInstanceTLAS(lightIdx, glm::translate(glm::mat4(1.0f), pos));
    }

    void RTRenderer::setPatchSize(uint8_t size)
    {
        if(!scene)
            throw std::runtime_error("Scene not initialized");
        scene->updatePatchData(size);
    }
}

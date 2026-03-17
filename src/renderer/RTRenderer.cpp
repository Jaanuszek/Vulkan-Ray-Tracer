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

        frameManager = std::make_unique<FrameManager>(ctx, swapChainManager);
        frameManager->init();

        scene = std::make_unique<Scene>(ctx);

        createScene();

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

    void RTRenderer::createScene()
    {
        VRTR_DEBUG("Creating scene");

        std::string viking_room_path = (CONSTANTS::ASSETS_DIR / "models/viking_room/").string();
        std::string viking_room_model_path = viking_room_path + "model/viking_room.obj";
        std::string viking_room_texture_path = viking_room_path + "textures/viking_room.png";

        scene->importModel(viking_room_model_path, viking_room_texture_path);

        std::string guy_model_path = (CONSTANTS::ASSETS_DIR / "models/guy/model/guy.obj").string();
        std::string guy_model_name = Model::getModelNameFromPath(guy_model_path);

        auto [floorVertices, floorIndices] = CustomModels::createRectangle();
        Material floorMat{
            .albedo = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f),
            .type = MaterialType::METALLIC,
        };

        glm::mat4 floorModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.1f, 0.0f));
        scene->addObject("floor", floorVertices, floorIndices, floorMat, floorModel);

        auto [wallVertices, wallIndices] = CustomModels::createRectangle();
        Material wallMat{
            .albedo = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
            .type = MaterialType::METALLIC,
        };
        glm::mat4 wallModel = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f));
        wallModel = glm::rotate(wallModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        wallModel = glm::translate(wallModel, glm::vec3(0.0f, -10.0f, -2.0f));

        scene->addObject("wall", wallVertices, wallIndices, wallMat, wallModel);

        glm::mat4 lightObjectModel = glm::translate(glm::mat4(1.0f), sceneSettings.ubo.light_pos);
        lightObjectModel = glm::scale(lightObjectModel, glm::vec3(0.2f));
        scene->addObject(LIGHT_MODEL_NAME, floorVertices, floorIndices, Material{
            .albedo = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .type = MaterialType::ALBEDO,
        }, lightObjectModel);

        // To musi byc na końcu
        scene->buildTLAS();
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
                scene->updateTLAS(deltaTime, sceneSettings.transformations.rotationAngle);
                gui->setUpdated(false);
            }
            updateUniformBuffer(); // Camera UBO update

            frameManager->submitQueue(submitCommandBuffers);
            frameManager->runCudaFrame(frameCount);
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

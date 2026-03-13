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

        vkCudaInteropManager = std::make_unique<vkCudaInterop>(ctx);
        vkCudaInteropManager->init();

        setupVMA();

        RayTracingPipeline::initRayTracing(ctx);

        swapChainManager = std::make_unique<SwapChainManager>(ctx);
        swapChainManager->init(window);

        commandBufferManager = std::make_shared<CommandBufferManager>(ctx);
        commandBufferManager->init();

        initImGUI(window);

        createSyncObjects();

        storageImage = std::make_shared<StorageImage>(ctx, width, height);
        storageImage->init(commandBufferManager->getCommandPool());

        scene = std::make_unique<Scene>(ctx);

        createScene();

        uniform_buffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, sizeof(UniformData),
                                            vk::BufferUsageFlagBits{},
                                            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                            vk::BufferUsageFlagBits2::eUniformBuffer | vk::BufferUsageFlagBits2::eShaderDeviceAddress);

        updateUniformBuffer();



        DescriptorResources dr{};
        dr.ubo = uniform_buffer->getBufferHandle();
        dr.storageImageView = storageImage->getImageViewHandle();
        scene->appendDescriptorResources(dr);
        vkCudaInteropManager->appendDescriptorResources(dr);

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

    void RTRenderer::updateUniformBuffer()
    {
        sceneSettings.ubo.proj_inverse = glm::inverse(camera->matrices.perspective);
        sceneSettings.ubo.view_inverse = glm::inverse(camera->matrices.view);
        sceneSettings.ubo.light_pos = sceneSettings.ubo.light_pos;
        uniform_buffer->Update(&sceneSettings.ubo, sizeof(UniformData));
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

        // To musi byc na końcu
        scene->buildTLAS();
    }

    void RTRenderer::recreateResources(GLFWwindow *window)
    {
        swapChainManager->recreateSwapChain(window, width, height);
        storageImage->recreate(commandBufferManager->getCommandPool(), width, height);

        // Update camera perspective with new aspect ratio
        camera->setPerspective(45.0f, static_cast<float>(width) / height, 0.1f, 100.0f);

        DescriptorResources dr{};
        dr.ubo = uniform_buffer->getBufferHandle();
        dr.storageImageView = storageImage->getImageViewHandle();
        scene->appendDescriptorResources(dr);
        vkCudaInteropManager->appendDescriptorResources(dr);

        rayTracingPipeline->updatePipelineDescriptors(dr, width, height);   
    }

    void RTRenderer::drawFrame(GLFWwindow *window, double deltaTime, bool renderGUI)
    {
        constexpr uint64_t timeout = 5'000'000'000;
        vk::raii::SwapchainKHR &swapChain = swapChainManager->getSwapChain();
        while (vk::Result::eTimeout == ctx.logicalDevice.waitForFences(*drawFences.at(commandBufferManager->getCurrentFrame()), VK_TRUE, timeout))
            ;

        // Unfortunately it needs to be inside try catch block, because "acquireNextImage" is throwing exceptions
        // I cant disable it, because i am using vk::raii and it requires exceptions to be enabled :(
        try
        {
            uint32_t presentSemaphoreIdx = commandBufferManager->getSemaphoreIndex();
            uint32_t frameIdx = commandBufferManager->getCurrentFrame();

            /* 
                acquireNextImage to jest asynchroniczna funkcja, która zwraca wyrenderowany obraz oraz jego indeks w swapchainie
                Należy ją zsynchronizować, podając semafor, lub/i fence
                Przez użyciem tego obrazu, należy poczekać na zasygnalizowanie semafora przez tą funkcje
                bo inaczej to jest UB
            */
            auto [result, imageIndex] = swapChain.acquireNextImage(timeout, presentCompleteSemaphores.at(presentSemaphoreIdx), nullptr);

            ctx.logicalDevice.resetFences({drawFences[frameIdx]});

            /*
                Zmienna która mówi w jakim etapie pipeline'u GPU powinien czekać na semafor z acquireNextImage
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT oznacza, czekaj przed wykonaniem jakiegokolwiek polecenia GPU
                żaden etap nie ruszy zanim seamfor będzie gotowy.
                TODO Nie jest to idealna flaga, pewnie bede musiał ją zmienić wp rzyszłości  
            */
            vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eAllCommands);

            // Tu są wykonywane jakieś polecenia CPU, które nie są asynchroniczne
            std::vector<vk::CommandBuffer> submitCommandBuffers = {*commandBufferManager->getCommandBuffer(imageIndex)};
            uint32_t submitCommandBufferCount = 1;
            if (renderGUI){
                gui->newFrame();

                vk::CommandBuffer guiCommandBuffer = gui->buildDrawCommandBuffer(
                    imageIndex,
                    swapChainManager->getSwapChainImage(imageIndex),
                    swapChainManager->getSwapChainImageView(imageIndex),
                    swapChainManager->getExtent());

                submitCommandBuffers.push_back(guiCommandBuffer);
                submitCommandBufferCount += (guiCommandBuffer != VK_NULL_HANDLE) ? 1u : 0u;
            }

            if(gui->updateRequired())
            {
                scene->updateTLAS(deltaTime, sceneSettings.transformations.rotationAngle);
                gui->setUpdated(false);
            }
            updateUniformBuffer(); // Camera UBO update
            
            /* 
                Tworzymy submitInfo który zawiera informacje:
                - Na jaką wartość semafora czekać
                - na jaki semafor czekać
                - na jakim etapie pipeline'u czekać
                - ilość command bufferów do wykonania
                - wskaźnik na command buffery do wykonania
                - ile semaforów zasygnalizować po wykonaniu tych command bufferów
                - jakie semafory zasygnalizować po wykonaniu tych command bufferów

                1. W tym przypadku czekamy na sygnał semafora presentCompleteSemaphore, który informuje
                czy obraz z acquireNextImage jest gotowy do użycia
                2. Sygnalizujemy semafor który pozwala na prezentacje obrazu na ekranie, czyli renderCompleteSemaphore
            */

            /*
                CUDA - synchronizacja
                Trzeba dodac kolejny semafor na ktory vulkan bedzie czekał i ktory bedzie sygnalizowany,
                by cuda mogła zacząć coś obliczać
                Dlatego dodaje tutaj do submit info dwa semafory: jeden na ktory czeka vulkan zeby wyswietlic obraz
                i drugi ktory czeka az cuda obliczy cos a nastepnie zasygnalizuje ten semafor
                zeby vulkan mogl wyswietlic obraz
            */

            static uint64_t cudaToVkWaitValue = 0;
            static uint64_t vkToCudaSignalValue = 1;

            std::array<vk::Semaphore, 2> waitSemaphores = {
                presentCompleteSemaphores.at(presentSemaphoreIdx),
                vkCudaInteropManager->getCudaCompleteSemaphore()
            };

            std::array<vk::Semaphore, 2> signalSemaphores = {
                renderCompleteSemaphores.at(frameIdx),
                vkCudaInteropManager->getCudaCompleteSemaphore()
            };

            std::array<uint64_t, 2> waitValues = {
                0,
                cudaToVkWaitValue
            };

            std::array<uint64_t, 2> signalValues = {
                0,
                vkToCudaSignalValue
            };

            std::array<vk::PipelineStageFlags, 2> waitStages = {
                vk::PipelineStageFlagBits::eAllCommands,
                vk::PipelineStageFlagBits::eAllCommands
            };

            vk::TimelineSemaphoreSubmitInfo timelineInfo{
                .waitSemaphoreValueCount = waitSemaphores.size(),
                .pWaitSemaphoreValues = waitValues.data(),
                .signalSemaphoreValueCount = signalSemaphores.size(),
                .pSignalSemaphoreValues = signalValues.data()
            };
            const vk::SubmitInfo submitInfo{
                .pNext = &timelineInfo,
                .waitSemaphoreCount = waitSemaphores.size(),
                .pWaitSemaphores = waitSemaphores.data(),
                .pWaitDstStageMask = waitStages.data(),
                .commandBufferCount = submitCommandBufferCount,
                .pCommandBuffers = submitCommandBuffers.data(),
                .signalSemaphoreCount = signalSemaphores.size(),
                .pSignalSemaphores = signalSemaphores.data()
            };
            /*   
                Ten kod poniżej, zapobiega freezowaniu systemu gdy Cuda nie zasygnalizuje semafora
                Vulkan ogólnie czeka w nieskonczonosc na sygnalizacje semafora w queue submit,
                Dlatego jak mamy deadlock, no to caly system sie blokuje xdd
                Dobrze by było dodac jakis mechanizm który wykrywa deadlock
                np jakis osobny thread który liczy czas od momentu zsubmitowania queue
                i jak osiagnie jakis timeout to konczy program

               uint64_t value;
               vkGetSemaphoreCounterValue(*ctx.logicalDevice, *cudaCompleteSemaphore, &value);
               if(value < cudaToVkWaitValue)
               {
                    // VRTR_INFO("Waiting for CUDA to finish...");
                    VRTR_WARN("Waiting for CUDA to finish... Semaphore value: {}, waiting for: {}", value, cudaToVkWaitValue);
                    // return;
                    exit(EXIT_FAILURE);
               }
            */

            /*
                Wysyłamy polecenia do wykonania na GPU, wraz z informacjami o synchronizacji
                Jeżeli queue zakończy wykonywanie poleceń, to sygnalizuje podany Fence
            */
           ctx.queue.submit({submitInfo}, *drawFences.at(frameIdx));

           /*
               Podobnie co poprzednio, tworzymy strukture z informacjami o synchronizacji,
               tym razem dla prezentacji obrazu na ekranie.
           */
           const vk::PresentInfoKHR presentInfoKHR{
               .pNext = nullptr,
               .waitSemaphoreCount = 1,
               .pWaitSemaphores = &*renderCompleteSemaphores.at(frameIdx),
               .swapchainCount = 1,
               .pSwapchains = &*swapChain,
               .pImageIndices = &imageIndex,
               .pResults = nullptr};

           /*
               Odpala kolejke prezentacji obrazu na ekranie
               Oczekuje, że obraz będzie VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
           */
           result = ctx.queue.presentKHR(presentInfoKHR);

           commandBufferManager->setSemaphoreIndex((presentSemaphoreIdx + 1) % presentCompleteSemaphores.size());
           commandBufferManager->setCurrentFrame((frameIdx + 1) % MAX_FRAMES_IN_FLIGHT);

           // === CUDA SYNC ===
           uint64_t cudaSemWait = vkToCudaSignalValue;
           uint64_t cudaSemSignal = vkToCudaSignalValue + 1;

           vkCudaInteropManager->waitForSemapore(cudaSemWait);
           vkCudaInteropManager->runKernel(frameCount);
           vkCudaInteropManager->signalSemaphore(cudaSemSignal);

           cudaToVkWaitValue = cudaSemSignal;
           vkToCudaSignalValue += 2;

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

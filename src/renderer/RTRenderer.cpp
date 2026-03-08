#include "RTRenderer.hpp"
#include "pch.h"

namespace VRTR
{
    RTRenderer::RTRenderer(std::shared_ptr<Camera> camera, SceneSettings &sceneSettings) 
    : camera(camera), sceneSettings(sceneSettings) {}

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
        gui.reset();

        vmaDestroyAllocator(ctx.vmaAllocator);
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

        setupCuda();

        setupVMA();

        initImGUI(window);

        RayTracingPipeline::initRayTracing(ctx);

        swapChainManager = std::make_unique<SwapChainManager>(ctx);
        swapChainManager->init(window);

        commandBufferManager = std::make_shared<CommandBufferManager>(ctx);
        commandBufferManager->init();

        gui->initResources(
            commandBufferManager->getCommandPool(),
            swapChainManager->getImageFormat(),
            static_cast<uint32_t>(swapChainManager->getSwapChainImages().size())
        );

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
        models.try_emplace("floor", std::make_unique<Model>(ctx, ctx.vmaAllocator, floorVertices, floorIndices, floorMat));
        uint32_t floorBlasIdx = asManager->createBLAS(models.at("floor"));

        glm::mat4 floorModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.1f, 0.0f));

        asManager->addInstance(floorBlasIdx, floorModel);
        modelInstanceOrder.push_back("floor");

        asManager->buildTLAS();

        // stworzenie storage buffora
        geometrySBO = std::make_unique<StorageBuffer>(ctx, ctx.vmaAllocator, sizeof(GeometryInfo));
        materialSBO = std::make_unique<StorageBuffer>(ctx, ctx.vmaAllocator, sizeof(Material));

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

        // TEMPORARY - Dodam jakiś prosty external buffor, zeby sprawdzic czy CUDA <-> Vulkan interop działa
        vk::DeviceSize cudaBuffSize = sizeof(glm::vec4);
        cudaInteropBuffer = std::make_unique<Buffer>(ctx, cudaBuffSize,
                                                     vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
                                                     vk::MemoryPropertyFlagBits::eDeviceLocal,
                                                     vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd);

        // to jest workaround, bo importCudaExtenralMemory potrzebuje lvalue
        // Ale nie powinien to byc problem, bo deviceMemory w vulkanie to ejst tylko uchwyt do pamięci,
        // wiec nie ma potrzeby przekazywania go jako referencje
        auto cudaBuffDevMem = cudaInteropBuffer->getBufferMemory();
        CUDA::importCudaExternalMemory(ctx.logicalDevice,(void**)&cudaData, cudaExternalMemory,
                                        cudaBuffDevMem, sizeof(glm::vec4), vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd);

        createExternalSemaphore(vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd);

        // vk::Semaphore vkSemaphoreHandle = *cudaCompleteSemaphore;
        CUDA::importCudaExternalSemaphore(ctx.logicalDevice,
                                          extCudaTimelineSemaphore,
                                          cudaCompleteSemaphore,
                                          vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd);

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

        vmaCreateAllocator(&allocatorCI, &ctx.vmaAllocator);
    }

    void RTRenderer::setupCuda()
    {
        auto deviceUUID = DeviceManager::getDeviceUUID(ctx.gpu);
        if(CUDA::initCUDA(deviceUUID.data(), VK_UUID_SIZE) < 0){
            VRTR_ERROR("Failed to initialize CUDA");
            exit(EXIT_FAILURE);
        }

        CUDA_CHECK_ERROR(cudaStreamCreateWithFlags(&cudaStream, cudaStreamNonBlocking));
    }

    void RTRenderer::initImGUI(GLFWwindow* window)
    {
        gui = std::make_unique<GUI>(ctx, sceneSettings);
        gui->init(window, width, height);
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

    // TODO to powinno byc w innym pliku
    void RTRenderer::createExternalSemaphore(vk::ExternalSemaphoreHandleTypeFlagBits handleType)
    {
        vk::ExportSemaphoreCreateInfo exportSemaphoreCreateInfo{
            .handleTypes = handleType
        };

        // Można zrobic semafory zwykle (ktore mają dwa stany albo signaled albo unsignaled)
        // Albo mozna zrobic timeline semaphores, które mają licznik 64 bitowy, pozwalający na ustawiaie kolejności
        // semaforów.
        // Timeline semafory brzmią ciekawie, eliminują potrzebe fenców,
        // i nie ma potrzeby tworzenia kazdego semaforu na klatke,
        // Wystarczyłby jeden semafor timeline, i dla kazdej klatki ustawić wartość tego semafora na jakąś kolejną wartość
        // np. dla klatki 0 ustawić semafor na 1, dla klatki 1 ustawić semafor na 2 itd.
#ifdef VK_TIMELINE_SEMAPHORE
        vk::SemaphoreTypeCreateInfo timelineCreateInfo{
            .semaphoreType = vk::SemaphoreType::eTimeline,
            .initialValue = 0
        };
        exportSemaphoreCreateInfo.pNext = &timelineCreateInfo;
#else
        exportSemaphoreCreateInfo.pNext = nullptr;
#endif
        vk::SemaphoreCreateInfo semaphoreCreateInfo{
            .pNext = &exportSemaphoreCreateInfo,
            .flags = {}
        };
        cudaCompleteSemaphore = vk::raii::Semaphore(ctx.logicalDevice, semaphoreCreateInfo);
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
            ctx.vmaAllocator,
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
        sceneSettings.ubo.proj_inverse = glm::inverse(camera->matrices.perspective);
        sceneSettings.ubo.view_inverse = glm::inverse(camera->matrices.view);
        sceneSettings.ubo.light_pos = sceneSettings.ubo.light_pos;
        uniform_buffer->Update(&sceneSettings.ubo, sizeof(UniformData));
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
                asManager->updateTLAS(deltaTime, sceneSettings.transformations.rotationAngle);
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
                cudaCompleteSemaphore
            };

            std::array<vk::Semaphore, 2> signalSemaphores = {
                renderCompleteSemaphores.at(frameIdx),
                cudaCompleteSemaphore
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
           cudaExternalSemaphoreWaitParams waitParams{};
           waitParams.flags = 0;
           waitParams.params.fence.value = cudaSemWait;
           waitParams.params.keyedMutex.timeoutMs = 5'000;

           cudaExternalSemaphoreSignalParams signalParams{};
           signalParams.flags = 0;
           signalParams.params.fence.value = cudaSemSignal;

           CUDA_CHECK_ERROR(cudaWaitExternalSemaphoresAsync(&extCudaTimelineSemaphore, &waitParams, 1, cudaStream));
           // Do something in cuda
           VRTR_INFO("CUDA timeline | vk wait: {}, vk signal: {}, cuda wait: {}, cuda signal: {}",
                     cudaToVkWaitValue, vkToCudaSignalValue, cudaSemWait, cudaSemSignal);
           CUDA_CHECK_ERROR(cudaSignalExternalSemaphoresAsync(&extCudaTimelineSemaphore, &signalParams, 1, cudaStream));

           cudaToVkWaitValue = cudaSemSignal;
           vkToCudaSignalValue += 2;
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

#include "pch.h"
#include "vkCudaInterop.hpp"

namespace VRTR
{
    vkCudaInterop::vkCudaInterop(RendererContext& ctx) : ctx(ctx) {}

    vkCudaInterop::~vkCudaInterop()
    {
        if(cudaExternalMemory)
        {
            CUDA_CHECK_ERROR(cudaDestroyExternalMemory(cudaExternalMemory));
        }

        if(extCudaTimelineSemaphore)
        {
            CUDA_CHECK_ERROR(cudaDestroyExternalSemaphore(extCudaTimelineSemaphore));
        }

        CUDA_CHECK_ERROR(cudaStreamDestroy(cudaStream));
    }

    void vkCudaInterop::init()
    {
        setupCuda();

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

        CUDA::importCudaExternalSemaphore(ctx.logicalDevice,
                                          extCudaTimelineSemaphore,
                                          cudaCompleteSemaphore,
                                          vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd);
    }

    void vkCudaInterop::runCudaFrame(uint64_t frameCount)
    {
        uint64_t cudaSemWait = vkToCudaSignalValue;
        uint64_t cudaSemSignal = vkToCudaSignalValue + 1;

        waitForSemapore(cudaSemWait);
        runKernel(frameCount);
        signalSemaphore(cudaSemSignal);

        cudaToVkWaitValue = cudaSemSignal;
        vkToCudaSignalValue += 2;
    }

    void vkCudaInterop::waitForSemapore(uint64_t waitValue)
    {
        cudaExternalSemaphoreWaitParams waitParams{};
        waitParams.flags = 0;
        waitParams.params.fence.value = waitValue;

        CUDA_CHECK_ERROR(cudaWaitExternalSemaphoresAsync(&extCudaTimelineSemaphore, &waitParams, 1, cudaStream));
    }

    void vkCudaInterop::signalSemaphore(uint64_t signalValue)
    {
        cudaExternalSemaphoreSignalParams signalParams{};
        signalParams.flags = 0;
        signalParams.params.fence.value = signalValue;

        CUDA_CHECK_ERROR(cudaSignalExternalSemaphoresAsync(&extCudaTimelineSemaphore, &signalParams, 1, cudaStream));
    }

    void vkCudaInterop::runKernel(uint64_t frameCount)
    {
        // Tutaj można odpalić jakiś kernel CUDA, który będzie coś robił z danymi w cudaData
        // Na potrzeby testów, można zrobić prosty kernel, który będzie zmieniał kolor na jakiś inny, zależnie od liczby klatki
        CUDA::stepSim(cudaData, frameCount, cudaStream);
    }

    void vkCudaInterop::appendDescriptorResources(DescriptorResources& resources)
    {
        resources.cudaColorBuffer = cudaInteropBuffer->getBufferHandle();
    }

    void vkCudaInterop::setupCuda()
    {
        VRTR_DEBUG("Setting up CUDA for Vulkan interop");
        auto deviceUUID = DeviceManager::getDeviceUUID(ctx.gpu);
        if(CUDA::initCUDA(deviceUUID.data(), VK_UUID_SIZE) < 0){
            VRTR_ERROR("Failed to initialize CUDA");
            exit(EXIT_FAILURE);
        }

        CUDA_CHECK_ERROR(cudaStreamCreateWithFlags(&cudaStream, cudaStreamNonBlocking));
    }

    void vkCudaInterop::createExternalSemaphore(vk::ExternalSemaphoreHandleTypeFlagBits handleType)
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
}
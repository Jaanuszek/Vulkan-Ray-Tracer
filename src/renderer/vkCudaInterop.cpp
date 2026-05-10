#include "pch.h"
#include "vkCudaInterop.hpp"
#include "VisibilityPipeline.hpp"

namespace VRTR
{
    namespace
    {
        // Must match visibility raygen dispatch width.
        constexpr uint32_t VISIBILITY_DISPATCH_RAYS_PER_PATCH = RAYS_PER_PATCH;

        inline float unshotMetric(const glm::vec3& e)
        {
            return e.r * 0.2126f + e.g * 0.7152f + e.b * 0.0722f;
        }
    }

    vkCudaInterop::vkCudaInterop(RendererContext& ctx, uint32_t passCount) : ctx(ctx), passCount(passCount) {}

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

    void vkCudaInterop::init(
        const std::vector<Patch>& patches,
        uint32_t vertexCount,
        const std::vector<uint32_t>& vertexPatchIndices, 
        const std::vector<uint32_t>& vertexPatchOffsets
    )
    {
        setupCuda();

        uint32_t emittingPatchCount = 0;
        float totalInitialUnshot = 0.0f;
        for (const auto& p : patches)
        {
            const float patchEnergy = unshotMetric(p.unshotEnergy);
            if (patchEnergy > 0.0f)
            {
                ++emittingPatchCount;
                totalInitialUnshot += patchEnergy;
            }
        }
        std::cout << "[CUDA init] patches=" << patches.size()
                  << " emittingPatches=" << emittingPatchCount
                  << " totalInitialUnshot=" << totalInitialUnshot << std::endl;

        cudaInteropBuffer = createCudaBuffer(
            sizeof(glm::vec4), 
            (void**)&cudaData, 
            cudaExternalMemory
        );

        cudaPatchesBuffer = createCudaBuffer(
            patches.size() * sizeof(Patch),
            (void**)&cudaPatchesData, 
            cudaPatchesExternalMemory
        );

        cudaSelectedPatchBuffer = createCudaBuffer(
            sizeof(SelectedPatch) * CUDA::SELECTED_PATCHES_COUNT, 
            (void**)&cudaSelectedPatchData, 
            cudaSelectedPatchExternalMemory
        );

        patchVisibilityCount = VISIBILITY_DISPATCH_RAYS_PER_PATCH * CUDA::SELECTED_PATCHES_COUNT;
        cudaPatchVisibilityBuffer = createCudaBuffer(
            sizeof(PatchVisibility) * patchVisibilityCount,
            (void**)&cudaPatchVisibilityData,
            cudaPatchVisibilityExternalMemory
        );

        cudaRadiosityLightmapBuffer = createCudaBuffer(
            sizeof(float4) * patches.size(),
            (void**)&cudaRadiosityLightmapData,
            cudaRadiosityLightmapExternalMemory
        );

        b_VertexPatchIndices = createCudaBuffer(
            sizeof(uint32_t) * vertexPatchIndices.size(),
            (void**)&d_VertexPatchIndices,
            e_VertexPatchIndices
        );

        b_VertexPatchOffsets = createCudaBuffer(
            sizeof(uint32_t) * vertexPatchOffsets.size(),
            (void**)&d_VertexPatchOffsets,
            e_VertexPatchOffsets
        );

        cudaVertexRadiosityBuffer = createCudaBuffer(
            sizeof(glm::vec3) * vertexCount,
            (void**)&cudaVertexRadiosityData,
            cudaVertexRadiosityExternalMemory
        );

        CUDA_CHECK_ERROR(cudaMemcpy(cudaPatchesData, patches.data(), patches.size() * sizeof(Patch), cudaMemcpyHostToDevice));
        CUDA_CHECK_ERROR(cudaMemset(cudaRadiosityLightmapData, 0, sizeof(float4) * patches.size()));
        CUDA_CHECK_ERROR(cudaMemcpy(d_VertexPatchIndices, vertexPatchIndices.data(), sizeof(uint32_t) * vertexPatchIndices.size(), cudaMemcpyHostToDevice));
        CUDA_CHECK_ERROR(cudaMemcpy(d_VertexPatchOffsets, vertexPatchOffsets.data(), sizeof(uint32_t) * vertexPatchOffsets.size(), cudaMemcpyHostToDevice));

        std::array<SelectedPatch, CUDA::SELECTED_PATCHES_COUNT> selectedInit{};
        for (auto& selected : selectedInit)
        {
            selected.patchId = 0xFFFFFFFF;
            selected.unshotEnergy = 0.0f;
        }
        CUDA_CHECK_ERROR(cudaMemcpy(cudaSelectedPatchData, selectedInit.data(), sizeof(SelectedPatch) * CUDA::SELECTED_PATCHES_COUNT, cudaMemcpyHostToDevice));

        CUDA_CHECK_ERROR(cudaMemset(cudaVertexRadiosityData, 0, sizeof(glm::vec3) * vertexCount));

        createExternalSemaphore(vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd);

        CUDA::importCudaExternalSemaphore(ctx.logicalDevice,
                                          extCudaTimelineSemaphore,
                                          cudaCompleteSemaphore,
                                          vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd);
    }

    // void vkCudaInterop::runCudaFrame(uint32_t patchesCount)
    // {
    //     static uint64_t debugFrameIdx = 0;

    //     uint64_t cudaSemWait = vkToCudaSignalValue;
    //     uint64_t cudaSemSignal = vkToCudaSignalValue + 1;

    //     waitForSemapore(cudaSemWait);

    //     // Reset selected patch przed kazda iteracja, zeby atomicMax liczyl od zera.
    //     std::array<SelectedPatch, CUDA::SELECTED_PATCHES_COUNT> selectedInit{};
    //     for (auto& selected : selectedInit)
    //     {
    //         selected.patchId = 0xFFFFFFFF;
    //         selected.unshotEnergy = 0.0f;
    //     }
    //     CUDA_CHECK_ERROR(cudaMemcpyAsync(cudaSelectedPatchData, selectedInit.data(), sizeof(SelectedPatch) * CUDA::SELECTED_PATCHES_COUNT,
    //                                      cudaMemcpyHostToDevice, cudaStream));

    //     CUDA::runFilterPatchesKernel(cudaPatchesData, patchesCount, cudaSelectedPatchData, cudaStream);

    //     // Debug readback: confirms whether CUDA kernel actually updates SelectedPatch.
    //     // CUDA_CHECK_ERROR(cudaStreamSynchronize(cudaStream));
    //     // SelectedPatch hostSelected{};
    //     // CUDA_CHECK_ERROR(cudaMemcpy(&hostSelected, cudaSelectedPatchData, sizeof(SelectedPatch), cudaMemcpyDeviceToHost));
    //     // if (debugFrameIdx < 120 || (debugFrameIdx % 120 == 0))
    //     // {
    //     //     std::cout << "[CUDA] frame=" << debugFrameIdx
    //     //               << " selectedPatchId=" << hostSelected.patchId
    //     //               << " unshotEnergy=" << hostSelected.unshotEnergy
    //     //               << " patchesCount=" << patchesCount << std::endl;
    //     // }
    //     // ++debugFrameIdx;

    //     signalSemaphore(cudaSemSignal);

    //     cudaToVkWaitValue = cudaSemSignal;
    //     vkToCudaSignalValue += 2;
    // }

    void vkCudaInterop::runCudaSelectPass(uint32_t patchesCount)
    {
        // czekam na semafory o indeksach 0, 4, 8 ...
        // Bo (0)FilterPatches (1) -> (1)visibilityPass (2) -> (2) radiosityCalculation (3) -> (3) renderPass (4) -> (4) filterPatches...
        waitForSemapore(filterPatchesWaitValue);

        std::array<SelectedPatch, CUDA::SELECTED_PATCHES_COUNT> selectedInit{};
        for (auto& selected : selectedInit)
        {
            selected.patchId = 0xFFFFFFFF;
            selected.unshotEnergy = 0.0f;
        }
        CUDA_CHECK_ERROR(cudaMemcpyAsync(cudaSelectedPatchData, selectedInit.data(), sizeof(SelectedPatch) * CUDA::SELECTED_PATCHES_COUNT,
                                         cudaMemcpyHostToDevice, cudaStream));

        CUDA::runFilterPatchesKernel(cudaPatchesData, patchesCount, cudaSelectedPatchData, cudaStream);

        CUDA_CHECK_ERROR(cudaStreamSynchronize(cudaStream));
        SelectedPatch hostSelected{};
        CUDA_CHECK_ERROR(cudaMemcpy(&hostSelected, cudaSelectedPatchData, sizeof(SelectedPatch), cudaMemcpyDeviceToHost));

        static uint64_t debugFrameIdx = 0;
        if (debugFrameIdx < 120 || (debugFrameIdx % 120 == 0))
        {
            std::cout << "[After filter CUDA] frame=" << debugFrameIdx
                      << " selectedPatchId=" << hostSelected.patchId
                      << " unshotEnergy=" << hostSelected.unshotEnergy
                      << " patchesCount=" << patchesCount << std::endl;
        }
        ++debugFrameIdx;

        signalSemaphore(filterPatchesSignalValue);
        lastFilterPatchesSignalValue = filterPatchesSignalValue;

        filterPatchesWaitValue += passCount;
        filterPatchesSignalValue += passCount;
    }

    void vkCudaInterop::runCudaPostVisibilityPass(uint32_t patchesCount, uint32_t vertexCount)
    {
        waitForSemapore(radiosityWaitValue);

        CUDA::runPostVisibilityKernel(
            cudaPatchesData,
            patchesCount,
            cudaSelectedPatchData,
            cudaPatchVisibilityData,
            patchVisibilityCount,
            cudaRadiosityLightmapData,
            cudaStream);

        // static uint64_t debugLightmapFrame = 0;
        // if (patchesCount > 0 && (debugLightmapFrame < 120 || (debugLightmapFrame % 120 == 0)))
        // {
        //     CUDA_CHECK_ERROR(cudaStreamSynchronize(cudaStream));

        //     SelectedPatch hostSelected{};
        //     CUDA_CHECK_ERROR(cudaMemcpy(&hostSelected, cudaSelectedPatchData, sizeof(SelectedPatch), cudaMemcpyDeviceToHost));

        //     if (hostSelected.patchId != 0xFFFFFFFF && hostSelected.patchId < patchesCount)
        //     {

        //         std::vector<PatchVisibility> hostVisibilities(VISIBILITY_DISPATCH_RAYS_PER_PATCH * CUDA::SELECTED_PATCHES_COUNT);
        //         CUDA_CHECK_ERROR(cudaMemcpy(hostVisibilities.data(), cudaPatchVisibilityData, sizeof(PatchVisibility) * patchVisibilityCount, cudaMemcpyDeviceToHost));

        //         for(uint32_t i = 0; i < patchVisibilityCount; ++i)
        //         {
        //             const auto& vis = hostVisibilities[i];
        //             std::cout << "    visibility srcPatchId=" << vis.srcPatchId
        //                       << " dstPatchId=" << vis.dstPatchId
        //                       << " visibility=" << vis.visibility
        //                       << std::endl;
        //         }
        //     }
        // }
        // ++debugLightmapFrame;

        CUDA::runInterpolateVertexKernel(
            cudaPatchesData,
            patchesCount,
            d_VertexPatchIndices,
            d_VertexPatchOffsets,
            vertexCount,
            cudaVertexRadiosityData
        );

        signalSemaphore(radiositySignalValue);
        lastRadiositySignalValue = radiositySignalValue;

        radiosityWaitValue += passCount;
        radiositySignalValue += passCount;
    }

    // void vkCudaInterop::runCudaInterpolateVertexColorsPass(uint32_t patchesCount, uint32_t vertexCount)
    // {
    //     // tutaj nie czekam na semafory bo to ma byc uruchomione po visibilityPass
    //     CUDA::runInterpolateVertexKernel(
    //         cudaPatchesData,
    //         patchesCount,
    //         d_VertexPatchIndices,
    //         d_VertexPatchOffsets,
    //         vertexCount,
    //         cudaVertexRadiosityData
    //     );
    // }

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

    // void vkCudaInterop::runKernel()
    // {
        // Tutaj można odpalić jakiś kernel CUDA, który będzie coś robił z danymi w cudaData
        // Na potrzeby testów, można zrobić prosty kernel, który będzie zmieniał kolor na jakiś inny, zależnie od liczby klatki
        // CUDA::stepSim(cudaData, cudaStream);
    // }

    void vkCudaInterop::appendDescriptorResources(DescriptorResources& resources)
    {
        resources.cudaColorBuffer = cudaInteropBuffer->getBufferHandle();
        resources.patchBuffer = cudaPatchesBuffer->getBufferHandle();
        resources.selectedPatchBuffer = cudaSelectedPatchBuffer->getBufferHandle();
        resources.patchVisibilityBuffer = cudaPatchVisibilityBuffer->getBufferHandle();
        resources.radiosityLightmapBuffer = cudaRadiosityLightmapBuffer->getBufferHandle();
        resources.vertexRadiosityBuffer = cudaVertexRadiosityBuffer->getBufferHandle();
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

    std::unique_ptr<Buffer> vkCudaInterop::createCudaBuffer(vk::DeviceSize size, void** cudaPtr, cudaExternalMemory_t& externalMemory)
    {
        auto buffer = std::make_unique<Buffer>(
                    ctx, 
                    size,
                    vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
                    vk::MemoryPropertyFlagBits::eDeviceLocal,
                    vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd
                );

        auto cudaBuffDevMem = buffer->getBufferMemory();

        CUDA::importCudaExternalMemory(
            ctx.logicalDevice,
            cudaPtr, 
            externalMemory,
            cudaBuffDevMem, 
            size,
            vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd
        );

        return buffer;
    }
}
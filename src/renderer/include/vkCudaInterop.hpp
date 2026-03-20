#pragma once

#include "Logger.hpp"
#include "buffer.hpp"
#include "init_cuda.cuh"
#include "Radiosity.cuh"
#include "DeviceManager.hpp"
#include "DescriptorManager.hpp"
#include "Model.hpp"

namespace VRTR
{
    class vkCudaInterop
    {
        public:
            vkCudaInterop(RendererContext& ctx);
            ~vkCudaInterop();

            void init(const std::vector<Patch>& patches);

            void runCudaFrame(uint32_t patchesCount);
            // void runCudaSelectPass(uint32_t patchesCount);
            // void runCudaPostVisibilityPass(uint32_t patchesCount);

            void appendDescriptorResources(DescriptorResources& resources);

            vk::Semaphore getCudaCompleteSemaphore() const { return *cudaCompleteSemaphore; }

            uint64_t getVkWaitValue() const { return cudaToVkWaitValue; }
            uint64_t getVkSignalValue() const { return vkToCudaSignalValue; }

        private:
            void setupCuda();

            void createExternalSemaphore(vk::ExternalSemaphoreHandleTypeFlagBits handleType);

            void waitForSemapore(uint64_t waitValue);

            void signalSemaphore(uint64_t signalValue);

            // void runKernel();

        private:
            RendererContext &ctx;

            cudaStream_t cudaStream;
            std::unique_ptr<Buffer> cudaInteropBuffer;
            glm::vec4 *cudaData{};
            cudaExternalMemory_t cudaExternalMemory{nullptr};

            // RADIOSITY
            // To jest wskaznik dod danych patchy na GPU. To bedzie wspoldzielone z vulkanem
            // std::vector<Patch> patchesData;
            std::unique_ptr<Buffer> cudaPatchesBuffer;
            Patch* cudaPatchesData{};
            cudaExternalMemory_t cudaPatchesExternalMemory{nullptr};
            // SelectedPatch selectedPatchData;
            std::unique_ptr<Buffer> cudaSelectedPatchBuffer;
            SelectedPatch* cudaSelectedPatchData{};
            cudaExternalMemory_t cudaSelectedPatchExternalMemory{nullptr};

            uint64_t cudaToVkWaitValue{0};
            uint64_t vkToCudaSignalValue{1};

            vk::raii::Semaphore cudaCompleteSemaphore{nullptr};
            cudaExternalSemaphore_t  extCudaTimelineSemaphore{nullptr};
    };
}

#pragma once

#include "Logger.hpp"
#include "buffer.hpp"
#include "init_cuda.cuh"
#include "DeviceManager.hpp"
#include "DescriptorManager.hpp"

namespace VRTR
{
    class vkCudaInterop
    {
        public:
            vkCudaInterop(RendererContext& ctx);
            ~vkCudaInterop();

            void init();

            void runCudaFrame(uint64_t frameCount);

            void appendDescriptorResources(DescriptorResources& resources);

            vk::Semaphore getCudaCompleteSemaphore() const { return *cudaCompleteSemaphore; }

            uint64_t getVkWaitValue() const { return cudaToVkWaitValue; }
            uint64_t getVkSignalValue() const { return vkToCudaSignalValue; }

        private:
            void setupCuda();

            void createExternalSemaphore(vk::ExternalSemaphoreHandleTypeFlagBits handleType);

            void waitForSemapore(uint64_t waitValue);

            void signalSemaphore(uint64_t signalValue);

            void runKernel(uint64_t frameCount);

        private:
            RendererContext &ctx;

            cudaStream_t cudaStream;
            std::unique_ptr<Buffer> cudaInteropBuffer;
            glm::vec4 *cudaData{};
            cudaExternalMemory_t cudaExternalMemory{nullptr};

            uint64_t cudaToVkWaitValue{0};
            uint64_t vkToCudaSignalValue{1};

            vk::raii::Semaphore cudaCompleteSemaphore{nullptr};
            cudaExternalSemaphore_t  extCudaTimelineSemaphore;
    };
}

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

            // Czy to dobry pomysl zeby oddzielac wait i signal?
            // Czy moze lepiej to zrobic w jednej funkcji wraz z wywolaniem kernela?
            void waitForSemapore(uint64_t waitValue);

            void signalSemaphore(uint64_t signalValue);

            // Trzeba jakos przekminic jak to zrobic zeby mozna bylo rozne kernele tu odpalić
            void runKernel(uint64_t frameCount);

            void appendDescriptorResources(DescriptorResources& resources);

            vk::Semaphore getCudaCompleteSemaphore() const { return *cudaCompleteSemaphore; }

        private:
            void setupCuda();

            void createExternalSemaphore(vk::ExternalSemaphoreHandleTypeFlagBits handleType);

        private:
            RendererContext &ctx;

            cudaStream_t cudaStream;
            std::unique_ptr<Buffer> cudaInteropBuffer;
            glm::vec4 *cudaData{};
            cudaExternalMemory_t cudaExternalMemory{nullptr};
            vk::raii::Semaphore cudaCompleteSemaphore{nullptr};
            cudaExternalSemaphore_t  extCudaTimelineSemaphore;
    };
}

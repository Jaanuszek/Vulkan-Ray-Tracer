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
            vkCudaInterop(RendererContext& ctx, uint32_t passCount);
            ~vkCudaInterop();

            void init(
                const std::vector<Patch>& patches,
                uint32_t vertexCount,
                const std::vector<uint32_t>& vertexPatchIndices, 
                const std::vector<uint32_t>& vertexPatchOffsets,
                const std::vector<uint32_t>& patchNeighborIndices,
                const std::vector<uint32_t>& patchNeighborOffsets
            );

            // void runCudaFrame(uint32_t patchesCount);
            void runCudaSelectPass(uint32_t patchesCount);
            void runCudaPostVisibilityPass(uint32_t patchesCount, uint32_t vertexCount);
            // void runCudaInterpolateVertexColorsPass(uint32_t patchesCount, uint32_t vertexCount);

            void appendDescriptorResources(DescriptorResources& resources);

            vk::Semaphore getCudaCompleteSemaphore() const { return *cudaCompleteSemaphore; }

            uint64_t getVkWaitValue() const { return cudaToVkWaitValue; }
            uint64_t getVkSignalValue() const { return vkToCudaSignalValue; }

            uint64_t getFPWaitValue() const { return filterPatchesWaitValue; }
            uint64_t getFPSignalValue() const { return lastFilterPatchesSignalValue; }

            uint64_t getRadiosityWaitValue() const { return radiosityWaitValue; }
            uint64_t getRadiositySignalValue() const { return lastRadiositySignalValue; }

            bool getCOVERAGED() const { return COVERAGED; }

        private:
            void setupCuda();

            void createExternalSemaphore(vk::ExternalSemaphoreHandleTypeFlagBits handleType);

            void waitForSemapore(uint64_t waitValue);

            void signalSemaphore(uint64_t signalValue);

            std::unique_ptr<Buffer> createCudaBuffer(vk::DeviceSize size, void** cudaPtr, cudaExternalMemory_t& externalMemory);

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

            std::unique_ptr<Buffer> cudaPatchVisibilityBuffer;
            PatchVisibility* cudaPatchVisibilityData{};
            cudaExternalMemory_t cudaPatchVisibilityExternalMemory{nullptr};
            uint32_t patchVisibilityCount{0};

            std::unique_ptr<Buffer> cudaRadiosityLightmapBuffer;
            float4* cudaRadiosityLightmapData{};
            cudaExternalMemory_t cudaRadiosityLightmapExternalMemory{nullptr};

            std::unique_ptr<Buffer> b_VertexPatchIndices;
            uint32_t* d_VertexPatchIndices{};
            cudaExternalMemory_t e_VertexPatchIndices{nullptr};

            std::unique_ptr<Buffer> b_VertexPatchOffsets;
            uint32_t* d_VertexPatchOffsets{};
            cudaExternalMemory_t e_VertexPatchOffsets{nullptr};

            std::unique_ptr<Buffer> b_PatchNeighborIndices;
            uint32_t* d_PatchNeighborIndices{};
            cudaExternalMemory_t e_PatchNeighborIndices{nullptr};

            std::unique_ptr<Buffer> b_PatchNeighborOffsets;
            uint32_t* d_PatchNeighborOffsets{};
            cudaExternalMemory_t e_PatchNeighborOffsets{nullptr};

            std::unique_ptr<Buffer> cudaDenoisedPatchRadiosityBuffer;
            glm::vec3* cudaDenoisedPatchRadiosityData{};
            cudaExternalMemory_t cudaDenoisedPatchRadiosityExternalMemory{nullptr};

            std::unique_ptr<Buffer> cudaVertexRadiosityBuffer;
            glm::vec3* cudaVertexRadiosityData{};
            cudaExternalMemory_t cudaVertexRadiosityExternalMemory{nullptr};

            uint32_t passCount;

            uint64_t cudaToVkWaitValue{0};
            uint64_t vkToCudaSignalValue{1};

            // Filter patches semaphores
            uint64_t filterPatchesWaitValue{0};
            uint64_t filterPatchesSignalValue{1};
            uint64_t lastFilterPatchesSignalValue{1};

            // Radiosity computation semaphores
            uint64_t radiosityWaitValue{2};
            uint64_t radiositySignalValue{3};
            uint64_t lastRadiositySignalValue{3};

            vk::raii::Semaphore cudaCompleteSemaphore{nullptr};
            cudaExternalSemaphore_t  extCudaTimelineSemaphore{nullptr};

            bool COVERAGED = false;
    };
}

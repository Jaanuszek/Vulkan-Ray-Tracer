#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"
#include "vkCudaInterop.hpp"
#include "vkFrameSync.hpp"
#include "SwapChainManager.hpp"

namespace VRTR
{
    constexpr uint64_t TIMEOUT = std::numeric_limits<uint64_t>::max();
    constexpr uint32_t PASS_COUNT = 4; // Filter Patches, Visibility Pass, Radiosity Pass, Render Pass

    class FrameManager
    {
        public:
            FrameManager(RendererContext &ctx, std::shared_ptr<SwapChainManager> swapChainManager);
            ~FrameManager() = default;

            void init(const std::vector<Patch>& patches, uint32_t vertexCount);

            /* AKA acquireNextFrame */
            uint32_t acquireNextImage();

            void submitVisibilityQueue(const std::vector<vk::CommandBuffer>& submitCommandBuffers);

            void submitRenderQueue(const std::vector<vk::CommandBuffer>& submitCommandBuffers);

            void presentFrame(uint32_t imageIndex);

            void runCudaFrame(uint32_t patchesCount);

            void runCudaSelectPass(uint32_t patchesCount);

            void runCudaPostVisibilityPass(uint32_t patchesCount);

            void appendDescriptorResources(DescriptorResources& resources)
            {
                vkCudaInteropManager->appendDescriptorResources(resources);
            }

            bool isComputeRadiosity() const { return computeRadiosity; }
            void setComputeRadiosity(bool value) { computeRadiosity = value; }

        private:
            void waitForFence();

        private:
            RendererContext &ctx;
            std::shared_ptr<SwapChainManager> swapChainManager;
            std::unique_ptr<vkFrameSync> frameSyncManager;
            std::unique_ptr<vkCudaInterop> vkCudaInteropManager;
            uint32_t activeImageIndex = 0;

            bool computeRadiosity = true;

        public:
            FrameManager() = delete;
    };
}
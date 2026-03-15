#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"

namespace VRTR
{
    class vkFrameSync
    {
        public:
            vkFrameSync(RendererContext& ctx);
            ~vkFrameSync();

            void init();

            vk::Semaphore getPresentCompleteSemaphore() const { return *presentCompleteSemaphores[currentFrame]; }
            vk::Semaphore getRenderCompleteSemaphore() const { return *renderCompleteSemaphores[currentFrame]; }
            vk::Fence getDrawFence() const { return *drawFences[currentFrame]; }
            void updateFrameIndex() { currentFrame = (currentFrame + 1) % CONSTANTS::MAX_FRAMES_IN_FLIGHT; }
            // void updateSemaphoreIndex() { semaphoreIndex = (semaphoreIndex + 1) % CONSTANTS::MAX_FRAMES_IN_FLIGHT; }

        private:
            void createSyncObjects();

        private:
            RendererContext &ctx;

            uint32_t currentFrame = 0;
            // uint32_t semaphoreIndex = 0;

            std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
            std::vector<vk::raii::Semaphore> renderCompleteSemaphores;
            std::vector<vk::raii::Fence> drawFences;
    };
}
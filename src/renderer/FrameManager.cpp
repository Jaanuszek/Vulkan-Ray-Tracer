#include "pch.h"
#include "FrameManager.hpp"

namespace VRTR
{
    FrameManager::FrameManager(RendererContext &ctx, std::shared_ptr<SwapChainManager> swapChainManager) 
        : ctx(ctx), swapChainManager(swapChainManager)
        {}

    void FrameManager::init(const std::vector<Patch>& patches)
    {
        frameSyncManager = std::make_unique<vkFrameSync>(ctx);
        frameSyncManager->init();

        vkCudaInteropManager = std::make_unique<vkCudaInterop>(ctx, PASS_COUNT);
        vkCudaInteropManager->init(patches);
    }
    uint32_t FrameManager::acquireNextImage()
    {
        waitForFence();

        auto [result, imageIndex] = swapChainManager->getSwapChain().acquireNextImage(TIMEOUT, frameSyncManager->getPresentCompleteSemaphore(), nullptr);

        if (result == vk::Result::eErrorOutOfDateKHR)
        {
            throw vk::OutOfDateKHRError("Swapchain out of date during image acquisition");
        }

        if (result == vk::Result::eSuboptimalKHR)
        {
            VRTR_WARN("Swap chain is suboptimal during image acquisition");
        }

        ctx.logicalDevice.resetFences(frameSyncManager->getDrawFence());

        activeImageIndex = imageIndex;

        return imageIndex;
    }

    void FrameManager::submitVisibilityQueue(const std::vector<vk::CommandBuffer>& submitCommandBuffers)
    {
        std::vector<vk::Semaphore> waitSemaphores{};
        std::vector<vk::Semaphore> signalSemaphores{};
        std::vector<uint64_t> waitValues{};
        std::vector<uint64_t> signalValues{};
        std::vector<vk::PipelineStageFlags> waitStages{};

        waitSemaphores = {
            vkCudaInteropManager->getCudaCompleteSemaphore()
        };

        signalSemaphores = {
            vkCudaInteropManager->getCudaCompleteSemaphore()
        };

        waitValues = {
            vkCudaInteropManager->getFPSignalValue()
        };

        signalValues = {
            vkCudaInteropManager->getRadiosityWaitValue()
        };

        waitStages = {
            vk::PipelineStageFlagBits::eAllCommands,
        };

        vk::TimelineSemaphoreSubmitInfo timelineInfo{
            .waitSemaphoreValueCount = static_cast<uint32_t>(waitSemaphores.size()),
            .pWaitSemaphoreValues = waitValues.data(),
            .signalSemaphoreValueCount = static_cast<uint32_t>(signalSemaphores.size()),
            .pSignalSemaphoreValues = signalValues.data()
        };

        const vk::SubmitInfo queueSubmitInfo{
            .pNext = &timelineInfo,
            .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
            .pWaitSemaphores = waitSemaphores.data(),
            .pWaitDstStageMask = waitStages.data(),
            .commandBufferCount = static_cast<uint32_t>(submitCommandBuffers.size()),
            .pCommandBuffers = submitCommandBuffers.data(),
            .signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()),
            .pSignalSemaphores = signalSemaphores.data()
        };

        ctx.queue.submit({queueSubmitInfo});
    }

    void FrameManager::submitRenderQueue(const std::vector<vk::CommandBuffer>& submitCommandBuffers)
    {
        std::vector<vk::Semaphore> waitSemaphores{};
        std::vector<vk::Semaphore> signalSemaphores{};
        std::vector<uint64_t> waitValues{};
        std::vector<uint64_t> signalValues{};
        std::vector<vk::PipelineStageFlags> waitStages{};

        if(computeRadiosity)
        {
            waitSemaphores = {
                frameSyncManager->getPresentCompleteSemaphore(),
                vkCudaInteropManager->getCudaCompleteSemaphore()
            };

            signalSemaphores = {
                frameSyncManager->getRenderCompleteSemaphore(),
                vkCudaInteropManager->getCudaCompleteSemaphore()
            };

            waitValues = {
                0,
                vkCudaInteropManager->getRadiositySignalValue()
            };

            signalValues = {
                0,
                vkCudaInteropManager->getFPWaitValue()
            };

            waitStages = {
                vk::PipelineStageFlagBits::eAllCommands,
                vk::PipelineStageFlagBits::eAllCommands
            };
        } 
        else
        {
            waitSemaphores = {
                frameSyncManager->getPresentCompleteSemaphore()
            };

            signalSemaphores = {
                frameSyncManager->getRenderCompleteSemaphore()
            };

            waitValues = {
                0
            };

            signalValues = {
                0
            };

            waitStages = {
                vk::PipelineStageFlagBits::eAllCommands
            };
        }

        // Jezeli semafor nie jest timeline, to vulkan ignoruje wartosc pWaitSemaphoreValues i pSignalSemaphoreValues
        // dla tego semafora, wiec to bedzie działać.
        vk::TimelineSemaphoreSubmitInfo timelineInfo{
            .waitSemaphoreValueCount = static_cast<uint32_t>(waitSemaphores.size()),
            .pWaitSemaphoreValues = waitValues.data(),
            .signalSemaphoreValueCount = static_cast<uint32_t>(signalSemaphores.size()),
            .pSignalSemaphoreValues = signalValues.data()
        };

        const vk::SubmitInfo queueSubmitInfo{
            .pNext = &timelineInfo,
            .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
            .pWaitSemaphores = waitSemaphores.data(),
            .pWaitDstStageMask = waitStages.data(),
            .commandBufferCount = static_cast<uint32_t>(submitCommandBuffers.size()),
            .pCommandBuffers = submitCommandBuffers.data(),
            .signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()),
            .pSignalSemaphores = signalSemaphores.data()
        };

        ctx.queue.submit({queueSubmitInfo}, frameSyncManager->getDrawFence());
    }

    void FrameManager::presentFrame(uint32_t imageIndex)
    {
        assert(imageIndex == activeImageIndex);

        std::array<vk::Semaphore, 1> presentWaitSemaphores = {
            frameSyncManager->getRenderCompleteSemaphore()
        };

        const vk::PresentInfoKHR presentInfoKHR{
            .pNext = nullptr,
            .waitSemaphoreCount = presentWaitSemaphores.size(),
            .pWaitSemaphores = presentWaitSemaphores.data(),
            .swapchainCount = 1,
            .pSwapchains = &*swapChainManager->getSwapChain(),
            .pImageIndices = &imageIndex,
            .pResults = nullptr};
        auto result = ctx.queue.presentKHR(presentInfoKHR);

        if (result == vk::Result::eErrorOutOfDateKHR)
        {
            throw vk::OutOfDateKHRError("Swapchain out of date during present");
        }

        if (result == vk::Result::eSuboptimalKHR)
        {
            VRTR_WARN("Swap chain is suboptimal");
        }

        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
        {
            throw std::runtime_error("Failed to present frame!");
        }

        frameSyncManager->updateFrameIndex();
    }

    void FrameManager::runCudaFrame(uint32_t patchesCount)
    {
        vkCudaInteropManager->runCudaFrame(patchesCount);
    }

    void FrameManager::runCudaSelectPass(uint32_t patchesCount)
    {
        vkCudaInteropManager->runCudaSelectPass(patchesCount);
    }

    void FrameManager::runCudaPostVisibilityPass()
    {
        vkCudaInteropManager->runCudaPostVisibilityPass();
    }

    void FrameManager::waitForFence()
    {
        while (vk::Result::eTimeout == ctx.logicalDevice.waitForFences(frameSyncManager->getDrawFence(), VK_TRUE, TIMEOUT))
        {
            ;
        }
    }


}
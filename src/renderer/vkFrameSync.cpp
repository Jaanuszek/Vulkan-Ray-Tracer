#include "pch.h"

#include "vkFrameSync.hpp"

namespace VRTR
{
    vkFrameSync::vkFrameSync(RendererContext& ctx) : ctx(ctx) {}

    vkFrameSync::~vkFrameSync()
    {
        VRTR_DEBUG("Destroying Frame Sync Objects");
        if (ctx.logicalDevice != nullptr)
        {
            ctx.logicalDevice.waitIdle();
        }
    }

    void vkFrameSync::init()
    {
        createSyncObjects();
    }

    void vkFrameSync::createSyncObjects()
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
        for (uint32_t i = 0; i < CONSTANTS::MAX_FRAMES_IN_FLIGHT; i++)
        {
            presentCompleteSemaphores.emplace_back(vk::raii::Semaphore(ctx.logicalDevice, semaphoreInfo));
            renderCompleteSemaphores.emplace_back(vk::raii::Semaphore(ctx.logicalDevice, semaphoreInfo));
            drawFences.emplace_back(vk::raii::Fence(ctx.logicalDevice, fenceInfo));
        }
    }
}
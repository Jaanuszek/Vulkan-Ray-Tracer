#include "pch.h"
#include "CommandBufferManager.hpp"

namespace VRTR
{
    CommandBufferManager::CommandBufferManager(RendererContext& ctx)
        : ctx(ctx)
    {}

    void CommandBufferManager::init()
    {
        createCommandPool();
        createCommandBuffers();
        createVisibilityCommandBuffers();
    }

    void CommandBufferManager::createCommandPool()
    {
        VRTR_DEBUG("Creating Command Pool");

        vk::CommandPoolCreateInfo poolInfo
        {
            .pNext = nullptr,
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = static_cast<uint32_t>(ctx.graphics_queue_index)
        };

        commandPool = vk::raii::CommandPool(ctx.logicalDevice, poolInfo);
    }

    void CommandBufferManager::createCommandBuffers()
    {
        VRTR_DEBUG("Creating Command Buffers");
        commandBuffers.clear();
        vk::CommandBufferAllocateInfo allocInfo
        {
            .pNext = nullptr,
            .commandPool = *commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = CONSTANTS::MAX_FRAMES_IN_FLIGHT
        };

        commandBuffers = vk::raii::CommandBuffers(ctx.logicalDevice, allocInfo);
    }

    void CommandBufferManager::createVisibilityCommandBuffers()
    {
        VRTR_DEBUG("Creating Visibility Command Buffers");
        VisibilityCommandBuffers.clear();
        vk::CommandBufferAllocateInfo allocInfo
        {
            .pNext = nullptr,
            .commandPool = *commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = CONSTANTS::MAX_FRAMES_IN_FLIGHT
        };

        VisibilityCommandBuffers = vk::raii::CommandBuffers(ctx.logicalDevice, allocInfo);
    }

    // vk::raii::CommandBuffer &CommandBufferManager::getCommandBuffer(uint32_t index)
    // {
    //     return commandBuffers.at(index);
    // }

    // vk::raii::CommandBuffer &CommandBufferManager::getVisibilityCommandBuffer(uint32_t index)
    // {
    //     return VisibilityCommandBuffers.at(index);
    // }

    void CommandBufferManager::transition_image_layout(vk::raii::CommandBuffer& commandBuffer,
                                                const vk::Image& image, 
                                                vk::ImageLayout oldLayout, 
                                                vk::ImageLayout newLayout,
                                                vk::AccessFlags2 srcAccessMask,
                                                vk::AccessFlags2 dstAccessMask,
                                                vk::PipelineStageFlags2 srcStageMask,
                                                vk::PipelineStageFlags2 dstStageMask,
                                                uint32_t baseMipLevel,
                                                uint32_t levelCount)
    {
        vk::ImageMemoryBarrier2 barrier
        {
            .pNext = nullptr,
            .srcStageMask = srcStageMask,
            .srcAccessMask = srcAccessMask,
            .dstStageMask = dstStageMask,
            .dstAccessMask = dstAccessMask,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor, 
                .baseMipLevel = baseMipLevel,
                .levelCount = levelCount,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        vk::DependencyInfo dependencyInfo
        {
            .pNext = nullptr,
            .dependencyFlags = {},
            .memoryBarrierCount = 0,
            .pMemoryBarriers = nullptr,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier
        };

        commandBuffer.pipelineBarrier2(dependencyInfo);
    }

    TempCMDBufferManager::TempCMDBufferManager(vk::raii::Device& device, vk::raii::Queue& queue, uint32_t graphicsQueueIndex)
        : device(device), queue(queue), graphicsQueueIndex(graphicsQueueIndex)
    {
        createTransientCommandPool();
    }

    vk::raii::CommandBuffer&  TempCMDBufferManager::createTempCmdBuffer()
    {
        // VRTR_DEBUG("Creating Transient Command Buffer");
        COMMANDS::beginSingleTimeCommands(transientCmdBuffer, device, transientCMDPool);

        return transientCmdBuffer;
    }

    void TempCMDBufferManager::submitAndWaitTempCmdBuffer()
    {
        // VRTR_DEBUG("Submitting and waiting for Transient Command Buffer");
        COMMANDS::endSingleTimeCommands(transientCmdBuffer, device, transientCMDPool, queue);
    }

    void TempCMDBufferManager::createTransientCommandPool()
    {
        const vk::CommandPoolCreateInfo cmdPoolCreateInfo{
            .flags = vk::CommandPoolCreateFlagBits::eTransient,
            .queueFamilyIndex = graphicsQueueIndex};
        transientCMDPool = vk::raii::CommandPool(device, cmdPoolCreateInfo);
    }

    void COMMANDS::beginSingleTimeCommands(vk::raii::CommandBuffer& cmd, vk::raii::Device& device, vk::raii::CommandPool& cmdPool)
    {
        //TODO add return value
        vk::CommandBufferAllocateInfo allocInfo
        {
            .commandPool = cmdPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };
        cmd = std::move(device.allocateCommandBuffers(allocInfo).front());

        cmd.begin(vk::CommandBufferBeginInfo{.flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    }

    void COMMANDS::endSingleTimeCommands(vk::raii::CommandBuffer& cmd, vk::raii::Device& device, vk::raii::CommandPool& cmdPool, vk::raii::Queue& queue)
    {
        // TODO add return value
        cmd.end();

        vk::FenceCreateInfo fenceInfo{};
        std::array<vk::raii::Fence, 1> fences = { device.createFence(fenceInfo) };


        vk::CommandBufferSubmitInfo commandBufferInfo{
            .commandBuffer = *cmd
        };

        std::array<vk::SubmitInfo2, 1> submitInfo{
            vk::SubmitInfo2{
                .commandBufferInfoCount = 1,
                .pCommandBufferInfos = &commandBufferInfo
            }
        };
        queue.submit2(submitInfo, fences.front());
        auto result = device.waitForFences(*fences.front(), VK_TRUE, UINT64_MAX);
        if(result != vk::Result::eSuccess)
        {
            VRTR_CRITICAL("Failed to wait for fence after storage image layout transition!");
            abort();
        }
    }
}
#include "pch.h"
#include "CommandBuffer.hpp"

namespace VRTR
{
    CommandBuffer::CommandBuffer(Context& ctx)
        : ctx(ctx)
    {}

    void CommandBuffer::createCommandPool()
    {
        VRTR_DEBUG("Creating Command Pool");

        vk::CommandPoolCreateInfo poolInfo
        {
            .pNext = nullptr,
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = static_cast<uint32_t>(ctx.graphics_queue_index)
        };

        ctx.commandPool = vk::raii::CommandPool(ctx.logicalDevice, poolInfo);
    }

    void CommandBuffer::createCommandBuffers()
    {
        VRTR_DEBUG("Creating Command Buffers");
        ctx.commandBuffers.clear();
        vk::CommandBufferAllocateInfo allocInfo
        {
            .pNext = nullptr,
            .commandPool = ctx.commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = MAX_FRAMES_IN_FLIGHT
        };

        // Allocate the command buffers (see commandBufferCount above)
        // std::move so we can move the lifetime to commandBuffer variable (ownership)
        ctx.commandBuffers = std::move(vk::raii::CommandBuffers(ctx.logicalDevice, allocInfo));
    }

    void CommandBuffer::transition_image_layout(const std::vector<vk::Image>& images, 
                                                uint32_t imageIndex,
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
            .image = images.at(imageIndex),
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

        ctx.commandBuffers.at(currentFrame).pipelineBarrier2(dependencyInfo);
    }

    void CommandBuffer::transition_image_layout(vk::raii::CommandBuffer& commandBuffer,
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

    vk::raii::CommandBuffer CommandBuffer::createTempCommandBuffer(Context& ctx, vk::CommandBufferLevel level, bool begin)
    {
        vk::CommandBufferAllocateInfo cmdBufferAllocInfo
        {
            .commandPool = ctx.commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };
        vk::raii::CommandBuffer cmdBuffer = std::move(ctx.logicalDevice.allocateCommandBuffers(cmdBufferAllocInfo).front());

        if(begin)
        {
            cmdBuffer.begin({});
        }

        return cmdBuffer;
    }

    void CommandBuffer::flushTempCommandBuffer(Context& ctx, 
                                vk::raii::CommandBuffer& commandBuffer, 
                                vk::raii::Queue* queue)
    {
        commandBuffer.end();
        vk::SubmitInfo submitInfo
        {
            .commandBufferCount = 1,
            .pCommandBuffers = &*commandBuffer,
        };

        vk::raii::Fence tmpFence = ctx.logicalDevice.createFence({});

        if(queue)
        {
            queue->submit({submitInfo}, tmpFence);
        }
        else
        {
            ctx.queue.submit({submitInfo}, tmpFence);
        }

        auto result = ctx.logicalDevice.waitForFences(*tmpFence, VK_TRUE, UINT64_MAX);
        if(result != vk::Result::eSuccess)
        {
            VRTR_CRITICAL("Failed to wait for fence after storage image layout transition!");
            abort();
        }
    }
}
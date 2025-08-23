#include "pch.h"
#include "CommandBuffer.hpp"

namespace VRTR
{
    CommandBuffer::CommandBuffer(vk::raii::CommandPool& commandPool, std::vector<vk::raii::CommandBuffer>& commandBuffers)
        : commandPool(commandPool), commandBuffers(commandBuffers)
    {}

    void CommandBuffer::createCommandPool(vk::raii::Device& device, 
                                          uint32_t queueFamilyIndex)
    {
        VRTR_DEBUG("Creating Command Pool");

        vk::CommandPoolCreateInfo poolInfo
        {
            .pNext = nullptr,
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = queueFamilyIndex
        };

        commandPool = vk::raii::CommandPool(device, poolInfo);
    }

    void CommandBuffer::createCommandBuffers(vk::raii::Device& device)
    {
        VRTR_DEBUG("Creating Command Buffers");
        commandBuffers.clear();
        vk::CommandBufferAllocateInfo allocInfo
        {
            .pNext = nullptr,
            .commandPool = commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = MAX_FRAMES_IN_FLIGHT
        };

        // Allocate the command buffers (see commandBufferCount above)
        // std::move so we can move the lifetime to commandBuffer variable (ownership)
        commandBuffers = std::move(vk::raii::CommandBuffers(device, allocInfo));
    }

    void CommandBuffer::transition_image_layout(const std::vector<vk::Image>& images, uint32_t imageIndex,
                                            vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                                            vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask,
                                            vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask,
                                            uint32_t baseMipLevel, uint32_t levelCount)
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

        commandBuffers.at(currentFrame).pipelineBarrier2(dependencyInfo);
    }
}
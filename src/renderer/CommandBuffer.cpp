#include "pch.h"
#include "CommandBuffer.hpp"

namespace VRTR
{
    CommandBuffer::CommandBuffer(vk::raii::CommandPool& commandPool, vk::raii::CommandBuffer& commandBuffer)
        : commandPool(commandPool), commandBuffer(commandBuffer)
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

    void CommandBuffer::createCommandBuffer(vk::raii::Device& device)
    {
        VRTR_DEBUG("Creating Command Buffer");

        vk::CommandBufferAllocateInfo allocInfo
        {
            .pNext = nullptr,
            .commandPool = commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };

        // Allocate the command buffers (see commandBufferCount above)
        // std::move so we can move the lifetime to commandBuffer variable (ownership)
        commandBuffer = std::move(vk::raii::CommandBuffers(device, allocInfo).front());
    }

    void CommandBuffer::recordCommandBuffer(uint32_t imageIndex)
    {
        VRTR_DEBUG("Recording Command Buffer");

        // .begin() wrapper function for vkBeginCommandBuffer
        commandBuffer.begin(
            {
                .pNext = nullptr,
                .flags = {},
                .pInheritanceInfo = nullptr // only relevant for secondary command buffers
            }
        );
    }

    void CommandBuffer::transition_image_layout(const std::vector<vk::Image>& images, uint32_t imageIndex,
                                            vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                                            vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask,
                                            vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask,
                                            uint32_t baseMipLevel = 0, uint32_t levelCount = 1)
    {
        vk::ImageMemoryBarrier2 barrier
        {
            .pNext = nullptr,
            .srcAccessMask = srcAccessMask,
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

        commandBuffer.pipelineBarrier2(dependencyInfo);
    }
}
#pragma once
#include "Logger.hpp"
#include "Constants.hpp"

namespace VRTR
{
    class CommandBuffer
    {
        public:
            CommandBuffer(vk::raii::CommandPool& commandPool, std::vector<vk::raii::CommandBuffer>& commandBuffers);
            ~CommandBuffer() = default;

            void createCommandPool(vk::raii::Device& device, uint32_t queueFamilyIndex);

            void createCommandBuffers(vk::raii::Device& device);

            void transition_image_layout(const std::vector<vk::Image>& images, uint32_t imageIndex,
                                            vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                                            vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask,
                                            vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask,
                                            uint32_t baseMipLevel = 0, uint32_t levelCount = 1);

        private:

        // In case of any problems with ownership, lets look here first!
        vk::raii::CommandPool& commandPool;
        std::vector<vk::raii::CommandBuffer>& commandBuffers;
    };
}
#pragma once
#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"

namespace VRTR
{
    class CommandBuffer
    {
        public:
            CommandBuffer(Context& ctx);
            ~CommandBuffer() = default;

            void createCommandPool();

            void createCommandBuffers();

            void transition_image_layout(const std::vector<vk::Image>& images, 
                                         uint32_t imageIndex,
                                         vk::ImageLayout oldLayout, 
                                         vk::ImageLayout newLayout,
                                         vk::AccessFlags2 srcAccessMask,
                                         vk::AccessFlags2 dstAccessMask,
                                         vk::PipelineStageFlags2 srcStageMask,
                                         vk::PipelineStageFlags2 dstStageMask,
                                         uint32_t baseMipLevel = 0,
                                         uint32_t levelCount = 1);

        private:
            Context& ctx;
    };
}
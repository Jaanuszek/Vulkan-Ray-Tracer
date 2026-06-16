#pragma once
#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"

namespace VRTR
{
    class CommandBufferManager
    {
    public:
        CommandBufferManager(RendererContext& ctx);

        void init();

        void createCommandPool();

        void createCommandBuffers();

        void createVisibilityCommandBuffers();

        vk::raii::CommandBuffer &getCommandBuffer(uint32_t index)
        {
            return commandBuffers.at(index);
        }

        vk::raii::CommandBuffer &getVisibilityCommandBuffer(uint32_t index)
        {
            return VisibilityCommandBuffers.at(index);
        }

        std::vector<vk::raii::CommandBuffer> &getVisibilityCommandBuffers() 
        { 
            return VisibilityCommandBuffers; 
        }
        
        inline std::vector<vk::raii::CommandBuffer> &getCommandBuffers()
        {
            return commandBuffers;
        }

        static void transition_image_layout(vk::raii::CommandBuffer &commandBuffer,
                                     const vk::Image &image,
                                     vk::ImageLayout oldLayout,
                                     vk::ImageLayout newLayout,
                                     vk::AccessFlags2 srcAccessMask,
                                     vk::AccessFlags2 dstAccessMask,
                                     vk::PipelineStageFlags2 srcStageMask,
                                     vk::PipelineStageFlags2 dstStageMask,
                                     uint32_t baseMipLevel = 0,
                                     uint32_t levelCount = 1);

        inline vk::raii::CommandPool& getCommandPool() { return commandPool; }

    private:
        RendererContext &ctx;

        vk::raii::CommandPool commandPool{nullptr};
        std::vector<vk::raii::CommandBuffer> commandBuffers{};
        std::vector<vk::raii::CommandBuffer> VisibilityCommandBuffers{};
    };

    // TODO moze oddzielny plik?

    class TempCMDBufferManager
    {
        public:
            TempCMDBufferManager(vk::raii::Device& device, vk::raii::Queue& queue, uint32_t graphicsQueueIndex);

            vk::raii::CommandBuffer& createTempCmdBuffer();
            void submitAndWaitTempCmdBuffer();

        private:
            void createTransientCommandPool();

        private:
            vk::raii::Device& device;
            vk::raii::Queue& queue;
            uint32_t graphicsQueueIndex;

            vk::raii::CommandPool transientCMDPool{nullptr};
            vk::raii::CommandBuffer transientCmdBuffer{nullptr};
    };

    namespace COMMANDS
    {
        void beginSingleTimeCommands(vk::raii::CommandBuffer& cmd, vk::raii::Device& device, vk::raii::CommandPool& cmdPool);
        void endSingleTimeCommands(vk::raii::CommandBuffer& cmd, vk::raii::Device& device, vk::raii::CommandPool& cmdPool, vk::raii::Queue& queue);
    }
}
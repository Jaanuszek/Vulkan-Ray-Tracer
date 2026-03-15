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

        vk::raii::CommandBuffer &getCommandBuffer(uint32_t index);
        
        inline std::vector<vk::raii::CommandBuffer> &getCommandBuffers()
        {
            return commandBuffers;
        }

        void beginCommandBuffer(uint32_t index, vk::CommandBufferBeginInfo beginInfo = {});

        void endCommandBuffer(uint32_t index);

        // void transition_image_layout(const std::vector<vk::Image> &images,
        //                              uint32_t imageIndex,
        //                              vk::ImageLayout oldLayout,
        //                              vk::ImageLayout newLayout,
        //                              vk::AccessFlags2 srcAccessMask,
        //                              vk::AccessFlags2 dstAccessMask,
        //                              vk::PipelineStageFlags2 srcStageMask,
        //                              vk::PipelineStageFlags2 dstStageMask,
        //                              uint32_t baseMipLevel = 0,
        //                              uint32_t levelCount = 1);

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
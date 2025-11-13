#pragma once
#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"

namespace VRTR
{
    constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    class CommandBufferManager
    {
    public:
        CommandBufferManager(vk::raii::Device& device, uint32_t graphicsQueueIndex);

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

        void transition_image_layout(const std::vector<vk::Image> &images,
                                     uint32_t imageIndex,
                                     vk::ImageLayout oldLayout,
                                     vk::ImageLayout newLayout,
                                     vk::AccessFlags2 srcAccessMask,
                                     vk::AccessFlags2 dstAccessMask,
                                     vk::PipelineStageFlags2 srcStageMask,
                                     vk::PipelineStageFlags2 dstStageMask,
                                     uint32_t baseMipLevel = 0,
                                     uint32_t levelCount = 1);

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

        // Temporary
        // bede chcial to przechowywac w oddzielnej klasie zajmującej się syncrhonizacją
        inline uint32_t getCurrentFrame() const { return currentFrame; }
        inline uint32_t getSemaphoreIndex() const { return semaphoreIndex; }
        inline void setCurrentFrame(uint32_t frame) { currentFrame = frame; }
        inline void setSemaphoreIndex(uint32_t index) { semaphoreIndex = index; }

    private:
        uint32_t currentFrame = 0;
        uint32_t semaphoreIndex = 0;

        vk::raii::Device& device;
        uint32_t graphicsQueueIndex;

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
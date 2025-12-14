#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"
#include "CommandBufferManager.hpp"
#include "buffer.hpp"

namespace VRTR
{
    class StorageImage
    {
        public:
            StorageImage(vk::raii::Device &logicalDevice, 
                vk::raii::PhysicalDevice &gpu,
                vk::raii::CommandPool &commandPool,
                vk::raii::Queue &queue,
                uint32_t graphics_queue_index,
                uint32_t width, uint32_t height);
            // TODO change it to something like this:
            // StorageImage(RendererContext &ctx, uint32_t width, uint32_t height);

            // ~StorageImage();
            void init();
            void recreate(uint32_t newWidth, uint32_t newHeight);
            vk::raii::Image &getImage() { return Image; };
            vk::raii::ImageView& getImageView() { return ImageView; };
            uint32_t getWidth() { return width; };
            uint32_t getHeight() { return height; };
        private:
            vk::raii::Device &logicalDevice;
            vk::raii::PhysicalDevice &gpu;
            vk::raii::CommandPool &commandPool;
            vk::raii::Queue &queue;
            uint32_t graphics_queue_index;
            uint32_t width;
            uint32_t height;
            vk::raii::Image Image{nullptr};
            vk::raii::ImageView ImageView{nullptr};
            vk::raii::DeviceMemory Memory{nullptr};
    };
}
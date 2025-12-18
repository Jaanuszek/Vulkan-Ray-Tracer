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
            StorageImage(RendererContext& ctx,
                uint32_t width, uint32_t height);

            void init(vk::raii::CommandPool& commandPool);
            void recreate(vk::raii::CommandPool& commandPool, uint32_t newWidth, uint32_t newHeight);
            vk::raii::Image &getImage() { return Image; };
            vk::raii::ImageView& getImageView() { return ImageView; };
            uint32_t getWidth() { return width; };
            uint32_t getHeight() { return height; };
        private:
            RendererContext &ctx;

            uint32_t width;
            uint32_t height;
            vk::raii::Image Image{nullptr};
            vk::raii::ImageView ImageView{nullptr};
            vk::raii::DeviceMemory Memory{nullptr};
    };
}
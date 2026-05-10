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

            void init();
            void recreate(uint32_t newWidth, uint32_t newHeight);
            const vk::raii::Image &getImage() const { return Image; };
            vk::Image getImageHandle() const { return *Image; };
            const vk::raii::ImageView& getImageView() const { return ImageView; };
            vk::ImageView getImageViewHandle() const { return *ImageView; };
            uint32_t getWidth() const { return width; };
            uint32_t getHeight() const { return height; };
        private:
            RendererContext &ctx;

            uint32_t width;
            uint32_t height;
            vk::raii::Image Image{nullptr};
            vk::raii::ImageView ImageView{nullptr};
            vk::raii::DeviceMemory Memory{nullptr};
    };
}
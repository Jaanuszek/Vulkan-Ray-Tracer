#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"
#include "VmaUsage.h"
#include "CommandBufferManager.hpp"

namespace VRTR
{
    class Image
    {
        public:
            Image(RendererContext& ctx, const vk::Format& format);
            ~Image();

            void createImage(vk::Extent3D extent, vk::ImageUsageFlags usage);
            void createImageMemory(vk::MemoryPropertyFlags properties);
            void createImageView(const vk::ImageAspectFlags& aspectFlags);
            
            void transitionImageLayout(vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
            void copyImageFromStagingToGPU(vk::Buffer stagingBuffer);

            vk::Image getImage() const { return *image; }
            vk::ImageView getImageView() const { return *imageView; }

        private:
            RendererContext &ctx;
            vk::raii::Image image{nullptr};
            vk::raii::ImageView imageView{nullptr};

            vk::Format format; // to jest reużywalne przez Image i ImageView
            VmaAllocation vmaAllocation{nullptr};
            vk::ImageCreateInfo imageCreateInfo{};
            bool imageCreateInfoInitialized{false};
    };
}
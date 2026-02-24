#pragma once

#include "Logger.hpp"
#include "buffer.hpp"
#include "ConstantsAndStructs.hpp"
#include "CommandBufferManager.hpp"

#include "stb_image.h"

namespace VRTR
{
    struct texData
    {
        int width;
        int height;
        int channels;
        vk::DeviceSize imageSize;
    };

    class Texture
    {
        public:
            Texture(RendererContext& ctx, const std::string &path);

            const vk::raii::ImageView& getTextureImageView() const { return textureImageView; }
            vk::ImageView getTextureImageViewHandle() const { return *textureImageView; }
            const vk::raii::Sampler& getTextureSampler() const { return texSampler; }
            vk::Sampler getTextureSamplerHandle() const { return *texSampler; }
            
        private:
            stbi_uc* loadTexture();
            void createTextureImage();
            void createTextureImageView();
            void createTextureSampler();
            void copyBufferToImage();
            void transitionImageLayout(vk::ImageLayout oldLayout, vk::ImageLayout newLayout); // wrapper for cmd buffer

        private:
            std::string texPath;
            texData textureData;
            RendererContext &ctx;
            std::unique_ptr<Buffer> stagingBuffer;
            vk::raii::Image textureImage{nullptr};
            vk::raii::DeviceMemory textureImageMemory{nullptr};
            vk::raii::ImageView textureImageView{nullptr};
            vk::raii::Sampler texSampler{nullptr};
    };
}
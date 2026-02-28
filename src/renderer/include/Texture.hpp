#pragma once

#include "Logger.hpp"
#include "buffer.hpp"
#include "ConstantsAndStructs.hpp"
#include "CommandBufferManager.hpp"
#include "Image.hpp"

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

            vk::ImageView getTextureImageViewHandle() const { return textureImage->getImageView(); }
            vk::Sampler getTextureSamplerHandle() const { return *texSampler; }
            
        private:
            stbi_uc* loadTexture();
            void createTextureSampler();
            void copyBufferToImage();

        private:
            std::string texPath;
            texData textureData;
            RendererContext &ctx;
            std::unique_ptr<Image> textureImage;
            vk::raii::Sampler texSampler{nullptr};
    };
}
#include "pch.h"
#include "Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace VRTR
{
    Texture::Texture(RendererContext& ctx, const std::string &path)
        : ctx(ctx), texPath(path)
    {
        auto pixels = loadTexture();
        // TODO zamienic to na VMA
        Buffer stagingBuffer(ctx, BufferType::STAGING, textureData.imageSize);
        stagingBuffer.Update(pixels, textureData.imageSize); // copy image data to staging buffer
        stbi_image_free(pixels);

        textureImage = std::make_unique<Image>(ctx, vk::Format::eB8G8R8A8Srgb);
        textureImage->createImage(vk::Extent3D{
            .width = static_cast<uint32_t>(textureData.width),
            .height = static_cast<uint32_t>(textureData.height),
            .depth = 1},
            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
        textureImage->createImageMemory(vk::MemoryPropertyFlagBits::eDeviceLocal);
        
        textureImage->transitionImageLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        textureImage->copyImageFromStagingToGPU(stagingBuffer.getBufferHandle());
        textureImage->transitionImageLayout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

        textureImage->createImageView(vk::ImageAspectFlagBits::eColor);
        createTextureSampler();
    }

    stbi_uc *Texture::loadTexture()
    {
        stbi_set_flip_vertically_on_load(true);
        stbi_uc* pixels = stbi_load(texPath.c_str(), &textureData.width, &textureData.height, &textureData.channels, STBI_rgb_alpha);

        assertm(pixels, "Failed to load texture image!");

        textureData.imageSize = textureData.width * textureData.height * 4;
        return pixels;
    }

    void Texture::createTextureSampler()
    {
        vk::PhysicalDeviceProperties properties = ctx.gpu.getProperties();
        vk::SamplerCreateInfo samplerIfo{
            .pNext = nullptr,
            .flags = {},
            .magFilter = vk::Filter::eLinear,
            .minFilter = vk::Filter::eLinear,
            .mipmapMode = vk::SamplerMipmapMode::eLinear,
            .addressModeU = vk::SamplerAddressMode::eRepeat,
            .addressModeV = vk::SamplerAddressMode::eRepeat,
            .addressModeW = vk::SamplerAddressMode::eRepeat,
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
            .compareEnable = VK_FALSE,
            .compareOp = vk::CompareOp::eAlways,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = vk::BorderColor::eIntOpaqueBlack,
            .unnormalizedCoordinates = VK_FALSE,
        };
        texSampler = vk::raii::Sampler(ctx.logicalDevice, samplerIfo);
    }
}
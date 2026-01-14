#include "pch.h"
#include "Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace VRTR
{
    Texture::Texture(RendererContext& ctx, const std::string &path)
        : ctx(ctx), texPath(path)
    {
        // createTextureImageView();
        auto pixels = loadTexture();
        stagingBuffer = std::make_unique<Buffer>(ctx, BufferType::STAGING, textureData.imageSize);
        stagingBuffer->Update(pixels, textureData.imageSize); // copy image data to staging buffer
        stbi_image_free(pixels);

        createTextureImage();
        
        transitionImageLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        copyBufferToImage();
        transitionImageLayout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

        createTextureImageView();
        createTextureSampler();
    }

    stbi_uc *Texture::loadTexture()
    {
        stbi_uc* pixels = stbi_load(texPath.c_str(), &textureData.width, &textureData.height, &textureData.channels, STBI_rgb_alpha);

        assertm(pixels, "Failed to load texture image!");

        textureData.imageSize = textureData.width * textureData.height * 4;
        return pixels;
    }

    void Texture::createTextureImage()
    {
        VRTR_DEBUG("Creating texture image from path: {}", texPath);

        vk::ImageCreateInfo imageInfo{
            .pNext = nullptr,
            .flags = {},
            .imageType = vk::ImageType::e2D,
            .format = vk::Format::eR8G8B8A8Srgb,
            .extent = vk::Extent3D{
                .width = static_cast<uint32_t>(textureData.width),
                .height = static_cast<uint32_t>(textureData.height),
                .depth = 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eOptimal,
            .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            .sharingMode = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined};

        textureImage = vk::raii::Image(ctx.logicalDevice, imageInfo);

        vk::MemoryRequirements memRequirements = textureImage.getMemoryRequirements();
        uint32_t memoryTypeIndex = Buffer::findMemoryType(ctx.gpu, memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
        vk::MemoryAllocateInfo allocInfo{
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = memoryTypeIndex};
        textureImageMemory = vk::raii::DeviceMemory(ctx.logicalDevice, allocInfo);
        textureImage.bindMemory(*textureImageMemory, 0);
    }

    void Texture::createTextureImageView()
    {
        VRTR_DEBUG("Creating texture image view");

        vk::ImageViewCreateInfo viewInfo{
            .image = *textureImage,
            .viewType = vk::ImageViewType::e2D,
            .format = vk::Format::eR8G8B8A8Srgb,
            .components = vk::ComponentMapping{
                .r = vk::ComponentSwizzle::eIdentity,
                .g = vk::ComponentSwizzle::eIdentity,
                .b = vk::ComponentSwizzle::eIdentity,
                .a = vk::ComponentSwizzle::eIdentity},
            .subresourceRange = vk::ImageSubresourceRange{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1}
            };
        textureImageView = vk::raii::ImageView(ctx.logicalDevice, viewInfo);
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

    void Texture::copyBufferToImage()
    {
        std::unique_ptr<TempCMDBufferManager> tempCmdBufferMgr = std::make_unique<TempCMDBufferManager>(ctx.logicalDevice, ctx.queue, ctx.graphics_queue_index);
        vk::raii::CommandBuffer& commandBuffer = tempCmdBufferMgr->createTempCmdBuffer();

        vk::BufferImageCopy region {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = vk::ImageSubresourceLayers{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1},
            .imageOffset = vk::Offset3D{0, 0, 0},
            .imageExtent = vk::Extent3D{
                .width = static_cast<uint32_t>(textureData.width),
                .height = static_cast<uint32_t>(textureData.height),
                .depth = 1}};

        commandBuffer.copyBufferToImage(stagingBuffer->getBuffer(), textureImage, vk::ImageLayout::eTransferDstOptimal, {region});
        tempCmdBufferMgr->submitAndWaitTempCmdBuffer();
    }

    void Texture::transitionImageLayout(vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
    {
        std::unique_ptr<TempCMDBufferManager> tempCmdBufferMgr = std::make_unique<TempCMDBufferManager>(ctx.logicalDevice, ctx.queue, ctx.graphics_queue_index);
        vk::raii::CommandBuffer& commandBuffer = tempCmdBufferMgr->createTempCmdBuffer();

        vk::AccessFlags2 srcAccessFlag;
        vk::AccessFlags2 dstAccessFlag;
        vk::PipelineStageFlags2 sourceStage;
        vk::PipelineStageFlags2 destinationStage;

        if(oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
        {
            srcAccessFlag = vk::AccessFlags2{};
            dstAccessFlag = vk::AccessFlagBits2::eTransferWrite;

            sourceStage = vk::PipelineStageFlagBits2::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits2::eTransfer;
        }
        else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            srcAccessFlag = vk::AccessFlagBits2::eTransferWrite;
            dstAccessFlag = vk::AccessFlagBits2::eShaderRead;

            sourceStage = vk::PipelineStageFlagBits2::eTransfer;
            destinationStage = vk::PipelineStageFlagBits2::eRayTracingShaderKHR;
        }
        else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }

        CommandBufferManager::transition_image_layout(
            commandBuffer,
            textureImage,
            oldLayout,
            newLayout,
            srcAccessFlag,
            dstAccessFlag,
            sourceStage,
            destinationStage
        );

        tempCmdBufferMgr->submitAndWaitTempCmdBuffer();
    }
}
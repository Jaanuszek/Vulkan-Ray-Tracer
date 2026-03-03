#include "pch.h"
#include "Image.hpp"

namespace VRTR
{
    Image::Image(RendererContext& ctx, const vk::Format& format)
        : ctx(ctx), format(format)
    {
    }

    Image::~Image()
    {
        if (vmaAllocation != nullptr && ctx.vmaAllocator != nullptr)
        {
            VkImage rawImage = *image ? static_cast<VkImage>(*image) : VK_NULL_HANDLE;
            if (rawImage != VK_NULL_HANDLE)
            {
                image.release();
            }
            vmaDestroyImage(ctx.vmaAllocator, rawImage, vmaAllocation);
            vmaAllocation = nullptr;
        }
    }

    void Image::createImage(vk::Extent3D extent, vk::ImageUsageFlags usage)
    {
        VRTR_DEBUG("Creating image");
        imageCreateInfo = vk::ImageCreateInfo{
        .pNext = nullptr,
        .flags = {},
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined};
        imageCreateInfoInitialized = true;
    }

    void Image::createImageMemory(vk::MemoryPropertyFlags properties)
    {
        VRTR_DEBUG("Creating image memory");

        if (!imageCreateInfoInitialized)
        {
            throw std::runtime_error("Image::createImage must be called before createImageMemory");
        }

        VmaAllocationCreateInfo allocCreateInfo{};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(properties);

        VkImage rawImage = VK_NULL_HANDLE;
        // vmaCreateImage poza stworzeniem image, alokuje pamiec i binduje ją
        VkResult result = vmaCreateImage(
            ctx.vmaAllocator,
            reinterpret_cast<VkImageCreateInfo*>(&imageCreateInfo),
            &allocCreateInfo,
            &rawImage,
            &vmaAllocation,
            nullptr
        );

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate and bind image memory with VMA");
        }

        image = vk::raii::Image(ctx.logicalDevice, rawImage);
    }

    void Image::createImageView(const vk::ImageAspectFlags& aspectFlags)
    {
        VRTR_DEBUG("Creating image view");
        vk::ImageViewCreateInfo viewInfo{
        .image = *image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .components = vk::ComponentMapping{
            .r = vk::ComponentSwizzle::eIdentity,
            .g = vk::ComponentSwizzle::eIdentity,
            .b = vk::ComponentSwizzle::eIdentity,
            .a = vk::ComponentSwizzle::eIdentity},
        .subresourceRange = vk::ImageSubresourceRange{
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1}
        };
        imageView = vk::raii::ImageView(ctx.logicalDevice, viewInfo);
    }

    void Image::transitionImageLayout(vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
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
            *image,
            oldLayout,
            newLayout,
            srcAccessFlag,
            dstAccessFlag,
            sourceStage,
            destinationStage
        );

        tempCmdBufferMgr->submitAndWaitTempCmdBuffer();
    }

    void Image::copyImageFromStagingToGPU(vk::Buffer stagingBuffer)
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
            .imageExtent = imageCreateInfo.extent
        };

        commandBuffer.copyBufferToImage(stagingBuffer, *image, vk::ImageLayout::eTransferDstOptimal, {region});
        tempCmdBufferMgr->submitAndWaitTempCmdBuffer();
    }
}
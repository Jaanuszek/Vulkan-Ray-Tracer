#include "pch.h"
#include "StorageImage.hpp"

namespace VRTR
{
    StorageImage::StorageImage(RendererContext& ctx,uint32_t width, uint32_t height)
        : ctx(ctx), width(width), height(height)
    {}

    void StorageImage::init(vk::raii::CommandPool& commandPool)
    {
        VRTR_DEBUG("Creating storage image");

        vk::ImageCreateInfo imgCreateInfo{
            .imageType = vk::ImageType::e2D,
            .format = vk::Format::eR8G8B8A8Unorm,
            .extent = vk::Extent3D{width, height, 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eOptimal,
            .usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc,
            .sharingMode = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined};
        Image = vk::raii::Image(ctx.logicalDevice, imgCreateInfo);

        vk::MemoryRequirements memRequirements = Image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = Buffer::findMemoryType(ctx.gpu, memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)};
        Memory = vk::raii::DeviceMemory(ctx.logicalDevice, allocInfo);
        Image.bindMemory(*Memory, 0); // Do I need this pointer here?

        vk::ImageViewCreateInfo viewCreateInfo{
            .image = *Image,
            .viewType = vk::ImageViewType::e2D,
            .format = vk::Format::eR8G8B8A8Unorm,
            .components = {
                vk::ComponentSwizzle::eIdentity, // it has to be identity inside storageImage
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity},
            .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
        ImageView = vk::raii::ImageView(ctx.logicalDevice, viewCreateInfo);

        // TODO OGARNAC TE TYMCZASOWE COMMAND BUFFERY
        vk::CommandBufferAllocateInfo cmdBufferAllocInfo{
            .commandPool = commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1};

        std::unique_ptr<TempCMDBufferManager> tempCmdBufferManager = std::make_unique<TempCMDBufferManager>(ctx.logicalDevice, ctx.queue, ctx.graphics_queue_index);
        vk::raii::CommandBuffer& tempCmdBuffer = tempCmdBufferManager->createTempCmdBuffer();

        CommandBufferManager::transition_image_layout(
            tempCmdBuffer,
            Image,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral, // it's basicaly storage image flag - we can do everything with it copy/write/read
            vk::AccessFlagBits2::eNone,
            vk::AccessFlagBits2::eShaderWrite,        // ?????????
            vk::PipelineStageFlagBits2::eAllCommands, // CHANGE IT LATER. It's very slow since GPU has to wait for all previous commands to finish
            vk::PipelineStageFlagBits2::eAllCommands  // CHANGE IT LATER
        );

        tempCmdBufferManager->submitAndWaitTempCmdBuffer();
    }

    void StorageImage::recreate(vk::raii::CommandPool& commandPool, uint32_t newWidth, uint32_t newHeight)
    {
        width = newWidth;
        height = newHeight;

        Image = nullptr;
        ImageView = nullptr;
        Memory = nullptr;

        init(commandPool);
    }
}
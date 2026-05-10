#include "pch.h"
#include "StorageBuffer.hpp"

namespace VRTR
{
    // TODO ajakby zrobic tu troche abstrakcji?
    // Zrobilbym interfejs Buffer
    // i klasy pochodne takie jak vertex buffer, index buffer, storage buffer itd
    // brzmi git hehe
    StorageBuffer::StorageBuffer(RendererContext& ctx, size_t typeSize)
        : ctx(ctx)
    {
        vk::BufferCreateInfo storageBufferCI{
            .sType = vk::StructureType::eBufferCreateInfo,
            .pNext = nullptr,
            .flags = {},
            .size = typeSize,
            .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            .sharingMode = vk::SharingMode::eExclusive,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr};

        VmaAllocationCreateInfo storageBufferAllocCI{
            .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        };
        VkBuffer rawStorageBuffer;
        vmaCreateBuffer(ctx.vmaAllocator, reinterpret_cast<VkBufferCreateInfo*>(&storageBufferCI), &storageBufferAllocCI, &rawStorageBuffer, &storageBufferAlloc, nullptr);
        storageBuffer = vk::raii::Buffer(ctx.logicalDevice, rawStorageBuffer);
    }

    StorageBuffer::~StorageBuffer()
    {
        if (storageBufferAlloc != nullptr)
        {
            VkBuffer rawStorageBuffer = static_cast<VkBuffer>(*storageBuffer);
            storageBuffer.release();
            vmaDestroyBuffer(ctx.vmaAllocator, rawStorageBuffer, storageBufferAlloc);
            storageBufferAlloc = nullptr;
        }
    }

    void StorageBuffer::copyDataToBuffer(const void *data, vk::DeviceSize size, vk::DeviceSize offset)
    {
        vmaCopyMemoryToAllocation(ctx.vmaAllocator, data, storageBufferAlloc, offset, size);
    }
}
#include "pch.h"
#include "StorageBuffer.hpp"

namespace VRTR
{
    StorageBuffer::StorageBuffer(RendererContext& ctx, VmaAllocator& vmaAlloc, size_t typeSize)
        : ctx(ctx), vmaAlloc(vmaAlloc)
    {
        // Dla kazdego Storage buffer, tworzymy tablice MAX_OBJECST elementów
        vk::DeviceSize bufferSize = typeSize * CONSTANTS::MAX_OBJECTS;

        vk::BufferCreateInfo storageBufferCI{
            .sType = vk::StructureType::eBufferCreateInfo,
            .pNext = nullptr,
            .flags = {},
            .size = bufferSize,
            .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            .sharingMode = vk::SharingMode::eExclusive,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr};

        VmaAllocationCreateInfo storageBufferAllocCI{
            .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        };
        VkBuffer rawStorageBuffer;
        vmaCreateBuffer(vmaAlloc, reinterpret_cast<VkBufferCreateInfo*>(&storageBufferCI), &storageBufferAllocCI, &rawStorageBuffer, &storageBufferAlloc, nullptr);
        storageBuffer = vk::raii::Buffer(ctx.logicalDevice, rawStorageBuffer);
    }

    StorageBuffer::~StorageBuffer()
    {
        vmaFreeMemory(vmaAlloc, storageBufferAlloc);
    }

    void StorageBuffer::copyDataToBuffer(const void *data, vk::DeviceSize size, vk::DeviceSize offset)
    {
        vmaCopyMemoryToAllocation(vmaAlloc, data, storageBufferAlloc, offset, size);
    }
}
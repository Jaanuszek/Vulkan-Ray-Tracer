#include "pch.h"
#include "buffer.hpp"

namespace VRTR
{
    Buffer::Buffer(vk::raii::Device &logicalDevice,
                   vk::raii::PhysicalDevice physicalDevice,
                   vk::DeviceSize size,
                   vk::BufferUsageFlags usage,
                   vk::MemoryPropertyFlags properties)
        : logDevice(logicalDevice)
    {
        vk::BufferCreateInfo bufferInfo = 
        {
            .pNext=nullptr,
            .flags={},
            .size=size,
            .usage=usage,
            .sharingMode=vk::SharingMode::eExclusive
        };

        buffer = vk::raii::Buffer{logicalDevice, bufferInfo};

        vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
        uint32_t memoryType = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

        // I will just hardcode it. I think I will always want to get device address
        vk::MemoryAllocateFlagsInfo allocateFlagsInfo{
            .pNext = nullptr,
            .flags = vk::MemoryAllocateFlagBits::eDeviceAddress,
            .deviceMask = 0
        };
        vk::MemoryAllocateInfo memoryAllocateInfo = 
        {
            .pNext=&allocateFlagsInfo,
            .allocationSize=memRequirements.size,
            .memoryTypeIndex=memoryType
        };

        bufferMemory = vk::raii::DeviceMemory{logicalDevice, memoryAllocateInfo};
        buffer.bindMemory(bufferMemory, 0);
    }

    Buffer::~Buffer()
    {

    }

    void Buffer::Update(const void* data, vk::DeviceSize size, vk::DeviceSize offset)
    {
        void* mappedData = bufferMemory.mapMemory(offset, size);
        memcpy(mappedData, data, static_cast<size_t>(size));
        bufferMemory.unmapMemory();
    }

    vk::DeviceAddress Buffer::getDeviceAddress()
    {
        vk::BufferDeviceAddressInfo addrInfo{};
        addrInfo.setBuffer(buffer);
        return logDevice.getBufferAddress(addrInfo);
    }

    uint32_t Buffer::findMemoryType(vk::raii::PhysicalDevice gpu, uint32_t typeFilter, vk::MemoryPropertyFlags properties)
    {
        for (uint32_t i = 0; i < gpu.getMemoryProperties().memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (gpu.getMemoryProperties().memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable memory type");
    }
}

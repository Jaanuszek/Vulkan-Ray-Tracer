#include "pch.h"
#include "buffer.hpp"

namespace VRTR
{
    Buffer::Buffer(vk::raii::Device &logicalDevice,
                   vk::raii::PhysicalDevice physicalDevice,
                   vk::DeviceSize size,
                   vk::BufferUsageFlags usage,
                   vk::MemoryPropertyFlags properties,
                   std::optional<vk::BufferUsageFlags2> usage2)
        : logDevice(logicalDevice)
    {
        if (usage2.has_value()) {
            vk::BufferUsageFlags2CreateInfo usageFlags2Info{
                .pNext = nullptr,
                .usage = *usage2
            };
            vk::BufferCreateInfo bufferInfo2 = 
            {
                .pNext=&usageFlags2Info,
                .flags={},
                .size=size,
                .usage= {}, // it's ignored in favor of usage2
                .sharingMode=vk::SharingMode::eExclusive
            };

            buffer = vk::raii::Buffer{logicalDevice, bufferInfo2};
        } else {
            vk::BufferCreateInfo bufferInfo = 
            {
                .pNext=nullptr,
                .flags={},
                .size=size,
                .usage=usage,
                .sharingMode=vk::SharingMode::eExclusive
            };
            buffer = vk::raii::Buffer{logicalDevice, bufferInfo};
        }

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
        // tu nie musi byc getBufferAddressKHR?
        deviceAddress = logicalDevice.getBufferAddress(vk::BufferDeviceAddressInfo{.buffer=buffer});
    }

    Buffer::Buffer(RendererContext& ctx,
                   BufferType type,
                   vk::DeviceSize size) : logDevice(ctx.logicalDevice)
    {
        switch(type)
        {
            case BufferType::VERTEX:
                {}
                break;
            case BufferType::INDEX:
                {}
                break;
            case BufferType::UNIFORM:
                {}
                break;
            case BufferType::STORAGE:
                {}
                break;
            case BufferType::ACCELERATION_STRUCTURE:
                {}
                break;
            case BufferType::SCRATCH:
                {
                    vk::BufferUsageFlags2CreateInfo bufferUsageFlags2Info{
                        .pNext = nullptr,
                        .usage = BufferTypeProperties[type].first
                    };

                    vk::BufferCreateInfo bufferCreateInfo
                    {
                        .pNext = &bufferUsageFlags2Info,
                        .size = size,
                        .usage = {},
                    };

                    buffer = vk::raii::Buffer(logDevice, bufferCreateInfo);

                    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

                    vk::MemoryAllocateFlagsInfo allocateFlagsInfo
                    {
                        .pNext = nullptr,
                        .flags = vk::MemoryAllocateFlagBits::eDeviceAddress,
                    };

                    uint32_t memoryType = Buffer::findMemoryType(ctx.gpu, memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

                    vk::MemoryAllocateInfo allocInfo
                    {
                        .pNext = &allocateFlagsInfo,
                        .allocationSize = memRequirements.size,
                        .memoryTypeIndex = memoryType
                    };
                    bufferMemory = vk::raii::DeviceMemory(ctx.logicalDevice, allocInfo);
                    buffer.bindMemory(*bufferMemory, 0);

                    deviceAddress = ctx.logicalDevice.getBufferAddress(vk::BufferDeviceAddressInfo{.buffer = buffer});
                }
                break;
            case BufferType::STAGING:
                {
                    vk::BufferCreateInfo bufferCreateInfo{
                        .pNext = nullptr,
                        .size = size,
                        .usage = vk::BufferUsageFlagBits::eTransferSrc,
                    };

                    buffer = vk::raii::Buffer(logDevice, bufferCreateInfo);

                    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
                    uint32_t memoryType = Buffer::findMemoryType(ctx.gpu, memRequirements.memoryTypeBits,
                                                                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
                    vk::MemoryAllocateInfo allocInfo
                    {
                        .pNext = nullptr,
                        .allocationSize = memRequirements.size, 
                        .memoryTypeIndex = memoryType
                    };
                    bufferMemory = vk::raii::DeviceMemory(logDevice, allocInfo);
                    buffer.bindMemory(*bufferMemory, 0);
                }
                break;
            default:
                break;
        }
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

    void* Buffer::map(vk::DeviceSize size, vk::DeviceSize offset)
    {
        return bufferMemory.mapMemory(offset, size);
    }

    void Buffer::unmap()
    {
        bufferMemory.unmapMemory();
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

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

    // I dont like this reference to VMAAlloc, but I need it to free the memory in destructor
    Buffer::Buffer(vk::raii::Device &logicalDevice,VmaAllocator& vmaAllocator, vk::DeviceSize size,
        vk::BufferUsageFlags usage, const VmaAllocationCreateInfo& allocInfo)
        : logDevice(logicalDevice), vmaAllocator(&vmaAllocator)
    {
        vk::BufferCreateInfo bufferCI{
        .sType = vk::StructureType::eBufferCreateInfo,
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
        };

        VkBuffer rawStorageBuffer;
        vmaCreateBuffer(vmaAllocator, reinterpret_cast<VkBufferCreateInfo*>(&bufferCI), &allocInfo, &rawStorageBuffer, &vmaAllocation, &vmaAllocationInfo);
        buffer = vk::raii::Buffer(logicalDevice, rawStorageBuffer);

        bufferInfo = {
            .usage = usage,
            .allocInfo = allocInfo,
            .size = size
        };
    }

    Buffer::~Buffer()
    {
        if (vmaAllocation != nullptr) {
            VkBuffer rawBuffer = static_cast<VkBuffer>(*buffer);
            buffer.release();
            vmaDestroyBuffer(*vmaAllocator, rawBuffer, vmaAllocation);
            vmaAllocation = nullptr;
        }
    }

    void Buffer::Update(const void* data, vk::DeviceSize size, vk::DeviceSize offset)
    {
        if(vmaAllocation == nullptr)
        {
            void* mappedData = bufferMemory.mapMemory(offset, size);
            memcpy(mappedData, data, static_cast<size_t>(size));
            bufferMemory.unmapMemory();
            return;
        }
        else {
            // Jezeli buffor nie jest stworzony z VMA_ALLOCATION_CREATE_MAPPED_BIT,
            // to korzystamy z wrappera VMA ktory mapuje pamiec, kopiuje ją i potem unmapuje
            if (!vmaAllocationInfo.pMappedData)
                vmaCopyMemoryToAllocation(*vmaAllocator, data, vmaAllocation, offset, size);
            else
                // void* nie pozwala na arytmetyke wskażników, wiec musze to zrzutować na uint8_t* zeby dodac offset
                // uint8_t*, char*, std::byte* to są bezpieczny typy do dostępu do dowolnych danych
                // czyli offset, musi byc podany w bajtach
                // uint8_t* p = reinterpret_cast<uint8_t*>(mappedPtr);
                // p + 5;  <---- to przesuwa wskaźnik o 5 bajtów (5 * sizeof(uint8_t))
                // float *f = reinterpret_cast<float*>(0x1000);
                // f + 1; <---- to przesuwa wskaźnik o 4 bajty (1 * sizeof(float))
                memcpy(static_cast<std::byte*>(vmaAllocationInfo.pMappedData) + offset, data, static_cast<size_t>(size));
        }
    }

    void* Buffer::map(vk::DeviceSize size, vk::DeviceSize offset)
    {
        return bufferMemory.mapMemory(offset, size);
    }

    void Buffer::unmap()
    {
        bufferMemory.unmapMemory();
    }

    void Buffer::recreate(vk::DeviceSize newSize)
    {
        // Mozliwe ze ta funkcja nie dziala poprawnie
        // assert(vmaAllocation == nullptr && "Recreate is not supported for buffers allocated without VMA");
        VkBuffer oldBuffer = static_cast<VkBuffer>(*buffer);
        buffer.release();
        vmaDestroyBuffer(*vmaAllocator, oldBuffer, vmaAllocation);
        vmaAllocation = nullptr;

        vk::BufferCreateInfo bufferCI{
            .pNext = nullptr,
            .size = newSize,
            .usage = bufferInfo.usage, // Uzywanie wczesniejszej wartosci struktury bufferInfo
            .sharingMode = vk::SharingMode::eExclusive
        };

        VkBuffer rawStorageBuffer;
        vmaCreateBuffer(*vmaAllocator, reinterpret_cast<VkBufferCreateInfo*>(&bufferCI), &bufferInfo.allocInfo, &rawStorageBuffer, &vmaAllocation, &vmaAllocationInfo);
        buffer = vk::raii::Buffer(logDevice, rawStorageBuffer);

        // Aktualizacja rozmiaru bez utraty usage i allocInfo
        bufferInfo.size = newSize;

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

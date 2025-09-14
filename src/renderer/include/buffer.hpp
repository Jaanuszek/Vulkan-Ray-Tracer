#pragma once
#include <Logger.hpp>

namespace VRTR
{
    class Buffer
    {
        public:
            Buffer(vk::raii::Device &logicalDevice,
                   vk::raii::PhysicalDevice physicalDevice,
                   vk::DeviceSize size,
                   vk::BufferUsageFlags usage,
                   vk::MemoryPropertyFlags properties);

            ~Buffer();
            
            void Update(const void* data, vk::DeviceSize size, vk::DeviceSize offset=0);

            vk::DeviceAddress getDeviceAddress();
            inline vk::raii::Buffer& getBuffer() { return buffer; }

            static uint32_t findMemoryType(vk::raii::PhysicalDevice gpu, uint32_t typeFilter, vk::MemoryPropertyFlags properties);

        private:
            vk::raii::Device& logDevice;
            vk::raii::Buffer buffer{nullptr};
            vk::raii::DeviceMemory bufferMemory{nullptr};
    };
}
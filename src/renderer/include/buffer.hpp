#pragma once
#include <Logger.hpp>
#include "ConstantsAndStructs.hpp"

namespace VRTR
{
    enum class BufferType
    {
        VERTEX,
        INDEX,
        UNIFORM,
        STORAGE,
        ACCELERATION_STRUCTURE,
        SCRATCH, // Temp buffor in GPU
        STAGING // TEMP buffor in CPU that can be used to transfer data to GPU
    };

    static std::unordered_map<BufferType, std::pair<vk::BufferUsageFlags2, vk::MemoryPropertyFlags>> BufferTypeProperties = {
        {
            BufferType::SCRATCH,
            {
                vk::BufferUsageFlagBits2::eStorageBuffer |
                vk::BufferUsageFlagBits2::eShaderDeviceAddress |
                vk::BufferUsageFlagBits2::eAccelerationStructureStorageKHR,
                vk::MemoryPropertyFlagBits::eDeviceLocal
            }
        },
        {
            BufferType::STORAGE,
            {
                vk::BufferUsageFlagBits2::eStorageBuffer |
                vk::BufferUsageFlagBits2::eShaderDeviceAddress,
                vk::MemoryPropertyFlagBits::eDeviceLocal
            }
        }
    };

    class Buffer
    {
    public:
        // Generic regular buffer constructor
        Buffer(vk::raii::Device &logicalDevice,
               vk::raii::PhysicalDevice physicalDevice,
               vk::DeviceSize size,
               vk::BufferUsageFlags usage,
               vk::MemoryPropertyFlags properties,
               std::optional<vk::BufferUsageFlags2> usage2 = std::nullopt);

        // Buffer constructor for specific types with hardcoded usage and memory properties
        Buffer(RendererContext &ctx,
               BufferType type,
               vk::DeviceSize size);

        ~Buffer();

        // It updates the buffer with given data
        void Update(const void *data, vk::DeviceSize size, vk::DeviceSize offset = 0);

        // Maps the buffer memory and returns pointer to it
        void *map(vk::DeviceSize size, vk::DeviceSize offset = 0);

        // Unmaps the buffer memory
        void unmap();

        // GETTERS
        inline const vk::raii::Buffer &getBuffer() const { return buffer; }

        vk::Buffer getBufferHandle() const { return *buffer; }

        inline vk::DeviceAddress getDeviceAddress() { return deviceAddress; }

        static uint32_t findMemoryType(vk::raii::PhysicalDevice gpu, uint32_t typeFilter, vk::MemoryPropertyFlags properties);

    private:
        vk::raii::Device &logDevice;
        vk::raii::Buffer buffer{nullptr};
        vk::raii::DeviceMemory bufferMemory{nullptr};
        vk::DeviceAddress deviceAddress{};
    };
}
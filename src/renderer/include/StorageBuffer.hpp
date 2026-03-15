#pragma once

#include "VmaUsage.h"
#include "ConstantsAndStructs.hpp"

namespace VRTR
{
    class StorageBuffer
    {
        public:
            StorageBuffer(RendererContext& ctx, vk::DeviceSize bufferSize);
            ~StorageBuffer();

            const vk::raii::Buffer& getBuffer() const { return storageBuffer; }
            vk::Buffer getBufferHandle() const { return *storageBuffer; }

            void copyDataToBuffer(const void *data, vk::DeviceSize size, vk::DeviceSize offset = 0);

        private:
            RendererContext& ctx;
            vk::raii::Buffer storageBuffer{nullptr};
            VmaAllocation storageBufferAlloc{nullptr};
    };
}
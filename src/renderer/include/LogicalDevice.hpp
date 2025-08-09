#pragma once

#include "Logger.hpp"

namespace VRTR
{
    class LogicalDevice
    {
        public:
            LogicalDevice() = default;
            ~LogicalDevice() = default;

            void createLogicalDevice(vk::raii::PhysicalDevice& physicalDevice, vk::raii::Device& logicalDevice,
                                     vk::raii::Queue& graphicsQueue);
        private:
    };
}
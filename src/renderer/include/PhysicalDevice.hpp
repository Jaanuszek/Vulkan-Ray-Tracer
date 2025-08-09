#pragma once
#include "Logger.hpp"

namespace VRTR
{
    static std::vector<const char*> deviceExtensions = 
    {
        vk::KHRSwapchainExtensionName
    };

    class PhysicalDevice
    {
        public:
        PhysicalDevice() = default;
        ~PhysicalDevice()= default;

        void pickPhysicalDevice(vk::raii::Instance& instance, vk::raii::PhysicalDevice& physicalDevice);
        static uint32_t findQueueFamilies(vk::raii::PhysicalDevice& physicalDevice);
        private:
        bool isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice);
    };
}
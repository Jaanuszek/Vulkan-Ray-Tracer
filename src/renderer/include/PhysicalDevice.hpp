#pragma once
#include "Logger.hpp"

namespace VRTR
{
    // struct QueueFamilyIndices
    // {
    //     std::optional<uint32_t> graphicsFamily;
    //     std::optional<uint32_t> presentFamily;

    //     bool isComplete() const
    //     {
    //         return graphicsFamily.has_value() && presentFamily.has_value();
    //     }
    // };
    
    static std::vector<const char*> deviceExtensions = 
    {
        vk::KHRSwapchainExtensionName,
        vk::KHRSpirv14ExtensionName
    };

    class PhysicalDevice
    {
        public:
        PhysicalDevice() = default;
        ~PhysicalDevice()= default;

        void pickPhysicalDevice(vk::raii::Instance& instance, vk::raii::PhysicalDevice& physicalDevice);


        static uint32_t findQueueFamilies(vk::raii::PhysicalDevice& physicalDevice, 
                                          vk::raii::SurfaceKHR& surface);


        private:
        bool isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice);
    };
}
#pragma once

#include "Logger.hpp"

namespace VRTR
{
    /*
        Struct created to hold Device properties, such as:
            - PhysicalDevice
            - Logical Device
            - Queue Families
    */

    struct DeviceProperties
    {
        vk::raii::PhysicalDevice physicalDevice{nullptr};
        vk::raii::SurfaceKHR surface{nullptr};
        vk::raii::Device logicalDevice{nullptr};
        vk::raii::Queue graphicsQueue{nullptr};
        uint32_t graphicsQueueFamilyIndex = -1;

        DeviceProperties() = default;
        DeviceProperties(const DeviceProperties&) = delete;
        DeviceProperties& operator=(const DeviceProperties&) = delete;
        DeviceProperties(DeviceProperties&&) = default;
        DeviceProperties& operator=(DeviceProperties&&) = default;
    };

    inline static std::vector<const char *> deviceExtensions // gpu logical device extensions
    {
        vk::KHRSwapchainExtensionName,
        vk::KHRSpirv14ExtensionName,
        vk::KHRRayQueryExtensionName,

        vk::KHRAccelerationStructureExtensionName,
        vk::KHRRayTracingPipelineExtensionName,
        vk::KHRDeferredHostOperationsExtensionName,
        vk::KHRExternalMemoryExtensionName,
        vk::KHRExternalSemaphoreExtensionName,
        vk::KHRTimelineSemaphoreExtensionName,
#ifdef _WIN64
        vk::KHRExternalMemoryWin32ExtensionName,
        vk::KHRExternalSemaphoreWin32ExtensionName,
#else
        vk::KHRExternalMemoryFdExtensionName,
        vk::KHRExternalSemaphoreFdExtensionName,
#endif
    };

    class DeviceManager
    {
        public:
            static DeviceProperties initDevice(GLFWwindow *window, vk::raii::Instance& instance);
            static std::array<uint8_t, VK_UUID_SIZE> getDeviceUUID(const vk::raii::PhysicalDevice &device);

        private:
            static std::vector<const char*> getRequiredExtensions();
            static bool isDeviceSuitable(const vk::raii::PhysicalDevice &device);
            static uint32_t findQueueFamilies(vk::raii::PhysicalDevice &device, vk::raii::SurfaceKHR &surface);
            static vk::raii::PhysicalDevice initPhysicalDevice(vk::raii::Instance& instance);
            static vk::raii::SurfaceKHR initSurface(GLFWwindow *window, vk::raii::Instance& instance);
            static std::pair<vk::raii::Device, vk::raii::Queue> initLogicalDevice(vk::raii::PhysicalDevice &device, uint32_t graphicsQueueFamilyIndex);
    };
};
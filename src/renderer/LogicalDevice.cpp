#include "pch.h"
#include "LogicalDevice.hpp"
#include "PhysicalDevice.hpp"

namespace VRTR
{
    void LogicalDevice::createLogicalDevice(vk::raii::PhysicalDevice& physicalDevice, vk::raii::Device& logicalDevice,
                                            vk::raii::Queue& graphicsQueue)
    {
        VRTR_DEBUG("CREATING LOGICAL DEVICE");

        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
        float queuePriority = 0.0f;
        uint32_t graphicsQueueFamilyIndex = PhysicalDevice::findQueueFamilies(physicalDevice);

        vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featuresChain
        {
            {},
            {.dynamicRendering = true},
            {.extendedDynamicState = true}
        };

        vk::DeviceQueueCreateInfo queueCreateInfo
        {
            .queueFamilyIndex = graphicsQueueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority 
        };
        
        // enabledLayerCount and ppEnabledLayersNames are not used in Vulkan 1.4
        vk::DeviceCreateInfo deviceCreateInfo
        {
            .pNext = &featuresChain.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
            .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
            .ppEnabledExtensionNames = deviceExtensions.data()
        };

        logicalDevice = vk::raii::Device(physicalDevice, deviceCreateInfo);
        graphicsQueue = vk::raii::Queue(logicalDevice, graphicsQueueFamilyIndex, 0);
    }
}
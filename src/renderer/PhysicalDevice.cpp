#include "PhysicalDevice.hpp"
#include "pch.h"

namespace VRTR
{
    void PhysicalDevice::pickPhysicalDevice(vk::raii::Instance& instance, vk::raii::PhysicalDevice& physicalDevice)
    {
        VRTR_DEBUG("PICKING PHYSICAL DEVICE");
        std::vector<vk::raii::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

        for (const auto& device : devices)
        {
            if (isDeviceSuitable(device))
            {
                physicalDevice = device;
                VRTR_DEBUG("Physical device selected: {}", physicalDevice.getProperties().deviceName.data());
                return;
            }
        }
    }

    bool PhysicalDevice::isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice)
    {
        bool isSuitable = false;
        vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
        std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();
        std::vector<vk::ExtensionProperties> availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();

        // Physical device needs to support Vulkan 1.4 or higher
        isSuitable = properties.apiVersion >= VK_API_VERSION_1_4;

        const auto& qfpIt = std::ranges::find_if(queueFamilies,
            [](const vk::QueueFamilyProperties& qfp)
            {
                return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlagBits>(0);
            });
        
        // Check if the device has at least one queue family that supports graphics operations
        isSuitable = isSuitable && (qfpIt != queueFamilies.end());

        bool foundExtensions = true;
        for (auto const& extension: deviceExtensions)
        {
            auto extensionIter = std::ranges::find_if(availableExtensions,
                [extension](const vk::ExtensionProperties& ep)
                {
                    return std::strcmp(ep.extensionName, extension) == 0;
                });

            foundExtensions = foundExtensions && (extensionIter != availableExtensions.end());
        }

        // Check if the device supports the required extensions
        isSuitable = isSuitable && foundExtensions;

        return isSuitable;
    }

    uint32_t PhysicalDevice::findQueueFamilies(vk::raii::PhysicalDevice& physicalDevice, vk::raii::SurfaceKHR& surface)
    {
        std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();
        uint32_t index = 0;

        for (const auto& queueFamily : queueFamilies)
        {
            // As of now, I will just look for a queue family that supports both graphics and presentation
            // I will need to update it later
            if ((queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) && 
                (physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(index), *surface)))
            {
                VRTR_DEBUG("Found graphics queue family that supports both graphics and presentation at index: {}", index);
                return index;
            }
            index++;
        }
        VRTR_CRITICAL("No suitable graphics queue family found!");
        throw std::runtime_error("No suitable graphics queue family found!");
    }
}
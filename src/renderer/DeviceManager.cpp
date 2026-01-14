#include "DeviceManager.hpp"
#include "pch.h"

namespace VRTR
{
    DeviceProperties DeviceManager::initDevice(GLFWwindow *window, vk::raii::Instance& instance)
    {
        DeviceProperties deviceProps;

        deviceProps.physicalDevice = initPhysicalDevice(instance);
        deviceProps.surface = initSurface(window, instance);
        deviceProps.graphicsQueueFamilyIndex = findQueueFamilies(deviceProps.physicalDevice, deviceProps.surface);
        auto [logicalDevice, graphicsQueue] = initLogicalDevice(deviceProps.physicalDevice, deviceProps.graphicsQueueFamilyIndex);
        deviceProps.logicalDevice = std::move(logicalDevice);
        deviceProps.graphicsQueue = std::move(graphicsQueue);
        return deviceProps;
    }

    bool DeviceManager::isDeviceSuitable(const vk::raii::PhysicalDevice &device)
    {
        // TODO implement device suitability checks
        return true;
    }

    uint32_t DeviceManager::findQueueFamilies(vk::raii::PhysicalDevice &device, vk::raii::SurfaceKHR &surface)
    {
       std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();
        uint32_t index = 0;

        for (const auto &queueFamily : queueFamilies)
        {
            // As of now, I will just look for a queue family that supports both graphics and presentation
            // I will need to update it later
            if ((queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) &&
                (device.getSurfaceSupportKHR(static_cast<uint32_t>(index), surface)))
            {
                VRTR_DEBUG("Found graphics queue family that supports both graphics and presentation at index: {}", index);
                return index;
            }
            index++;
        }
        VRTR_CRITICAL("No suitable graphics queue family found!");
        throw std::runtime_error("No suitable graphics queue family found!");
    }

    vk::raii::PhysicalDevice DeviceManager::initPhysicalDevice(vk::raii::Instance& instance)
    {
        VRTR_DEBUG("Selecting Physical Device");
        std::vector<vk::raii::PhysicalDevice> gpus = instance.enumeratePhysicalDevices();

        vk::raii::PhysicalDevice selectedDevice{nullptr};

        for (const auto &gpu : gpus)
        {
            if (isDeviceSuitable(gpu))
            {
                selectedDevice = gpu;
                VRTR_DEBUG("Physical device selected: {}", selectedDevice.getProperties().deviceName.data());
                break;
            }
        }
        return selectedDevice;
    }

    vk::raii::SurfaceKHR  DeviceManager::initSurface(GLFWwindow *window, vk::raii::Instance& instance)
    {
        VRTR_DEBUG("Selectring Surface");
        if(glfwVulkanSupported() != GLFW_TRUE)
        {
            VRTR_CRITICAL("GLFW VULKAN NOT SUPPORTED");
            throw std::runtime_error("GLFW VULKAN NOT SUPPORTED");
        }

        VkSurfaceKHR tempSurface;

        if (glfwCreateWindowSurface(*instance, window, nullptr, &tempSurface) != VK_SUCCESS)
        {
            VRTR_ERROR("Failed to create window surface");
            throw std::runtime_error("Failed to create window surface");
        }
        return vk::raii::SurfaceKHR(instance, tempSurface);
    }

    std::pair<vk::raii::Device, vk::raii::Queue>  DeviceManager::initLogicalDevice(vk::raii::PhysicalDevice &device, uint32_t graphicsQueueFamilyIndex)
    {
        VRTR_DEBUG("CREATING LOGICAL DEVICE");

        vk::StructureChain<vk::PhysicalDeviceFeatures2,
                           vk::PhysicalDeviceVulkan11Features,
                           vk::PhysicalDeviceVulkan13Features,
                           vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
                           vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
                           vk::PhysicalDeviceRayQueryFeaturesKHR,
                           vk::PhysicalDeviceBufferDeviceAddressFeatures,
                           vk::PhysicalDeviceAccelerationStructureFeaturesKHR>
            featuresChain{
                {.features = {.samplerAnisotropy = VK_TRUE}},
                {.shaderDrawParameters = VK_TRUE},
                {.synchronization2 = VK_TRUE,
                 .dynamicRendering = VK_TRUE},
                {.extendedDynamicState = VK_TRUE},
                {.rayTracingPipeline = VK_TRUE},
                {.rayQuery = VK_TRUE},
                {.bufferDeviceAddress = VK_TRUE},
                {.accelerationStructure = VK_TRUE}};

        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = device.getQueueFamilyProperties();
        float queuePriority = 0.0f;
        vk::DeviceQueueCreateInfo queueCreateInfo{
            .queueFamilyIndex = static_cast<uint32_t>(graphicsQueueFamilyIndex),
            .queueCount = 1,
            .pQueuePriorities = &queuePriority};

        // enabledLayerCount and ppEnabledLayersNames are not used in Vulkan 1.4
        vk::DeviceCreateInfo deviceCreateInfo{
            .pNext = &featuresChain.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
            .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
            .ppEnabledExtensionNames = deviceExtensions.data()};

        vk::raii::Device logicalDevice{device, deviceCreateInfo};
        vk::raii::Queue graphicsQueue = logicalDevice.getQueue(graphicsQueueFamilyIndex, 0);

        return std::make_pair(std::move(logicalDevice), std::move(graphicsQueue));
    }
}
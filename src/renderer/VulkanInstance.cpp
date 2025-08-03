#include "VulkanInstance.hpp"
#include "pch.h"

namespace VRTR
{
    void VulkanInstance::init()
    {
        VRTR_DEBUG("CREATING VULKAN INSTANCE");
        if(glfwVulkanSupported() != GLFW_TRUE)
        {
            VRTR_CRITICAL("GLFW VULKAN NOT SUPPORTED");
            throw std::runtime_error("GLFW VULKAN NOT SUPPORTED");
        }

        if(enableValidationLayers && !ValidationLayers::checkLayerValidationSupport())
        {
            VRTR_CRITICAL("Validation layer not available!");
            throw std::runtime_error("Validation layer not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Vulkan Ray tracer with radiosity";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if(enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            ValidationLayers::populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.ppEnabledLayerNames = nullptr;
            createInfo.pNext = nullptr;
        }
    
        auto extensions = getRequiredExtensions();
        uint32_t extensionsCount = static_cast<uint32_t>(extensions.size());
        createInfo.enabledExtensionCount = extensionsCount;
        createInfo.ppEnabledExtensionNames = extensions.data();

        uint32_t instanceExtensionsCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionsCount, nullptr);
        std::vector<VkExtensionProperties> extensionProperties(instanceExtensionsCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionsCount, extensionProperties.data());
        if(!checkExtensionsSupport(extensions.data(), extensionsCount, extensionProperties))
            VRTR_CRITICAL("GLFW EXTENSION DOES NOT MACH INSTANCE EXTENSIONS");

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
           throw std::runtime_error("failed to create instance!");
        }
    }

    void VulkanInstance::destroy()
    {
        VRTR_DEBUG("DESTROYING VULKAN INSTANCE");
        vkDestroyInstance(instance, nullptr);
    }

    bool VulkanInstance::checkExtensionsSupport(const char** glfwExtentions, uint32_t glfwExtensionCount, 
        const std::vector<VkExtensionProperties>& extensionsProperties)
    {
        for(uint32_t extension = 0;  extension < glfwExtensionCount; extension++)
        {
            const char* requiredExtension = glfwExtentions[extension];
            auto it = std::find_if(
                extensionsProperties.begin(),
                extensionsProperties.end(),
                [requiredExtension](const VkExtensionProperties& prop)
                {
                    return (strcmp(requiredExtension, prop.extensionName) == 0);
                }
            );

            if(it == extensionsProperties.end())
            {
                return false;
            }
        }
        return true;
    }

    std::vector<const char*> VulkanInstance::getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if(enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    } 
}
#include "VulkanInstance.hpp"

namespace VRTR
{
    VulkanInstance::VulkanInstance()
    {
        VRTR_DEBUG("CREATING VULKAN INSTANCE");
        if(glfwVulkanSupported() != GLFW_TRUE)
        {
            VRTR_CRITICAL("GLFW VULKAN NOT SUPPORTED");
            exit(0);
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
        createInfo.enabledExtensionCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;
        createInfo.enabledLayerCount = 0;

        uint32_t instanceExtensionsCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionsCount, nullptr);
        std::vector<VkExtensionProperties> extensionProperties(instanceExtensionsCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionsCount, extensionProperties.data());

        if(!checkExtensionsSupport(glfwExtensions, glfwExtensionCount, extensionProperties))
            VRTR_CRITICAL("GLFW EXTENSION DOES NOT MACH INSTANCE EXTENSIONS");

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
           throw std::runtime_error("failed to create instance!");
        }
    }
    VulkanInstance::~VulkanInstance()
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
}
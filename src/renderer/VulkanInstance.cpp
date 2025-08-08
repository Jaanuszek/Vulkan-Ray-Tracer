#include "VulkanInstance.hpp"
#include "pch.h"

namespace VRTR
{
    void VulkanInstance::init()
    {
        createInstance();
    }

    void VulkanInstance::destroy()
    {
        VRTR_DEBUG("DESTROYING VULKAN INSTANCE");
        // vkDestroyInstance(instance, nullptr);
    }

    void VulkanInstance::createInstance()
    {
        VRTR_DEBUG("CREATING VULKAN INSTANCE");
        if(glfwVulkanSupported() != GLFW_TRUE)
        {
            VRTR_CRITICAL("GLFW VULKAN NOT SUPPORTED");
            throw std::runtime_error("GLFW VULKAN NOT SUPPORTED");
        }

        // if(enableValidationLayers && !ValidationLayers::checkLayerValidationSupport())
        // {
        //     VRTR_CRITICAL("Validation layer not available!");
        //     throw std::runtime_error("Validation layer not available!");
        // }

        uint32_t ApiVersion = 0;
        if(vk::enumerateInstanceVersion(&ApiVersion) != vk::Result::eSuccess)
        {
            VRTR_CRITICAL("Can't enumerate instance version");
            throw std::runtime_error("Can't enumerate instance version");
        }

        uint32_t major = vk::apiVersionMajor(ApiVersion);
        uint32_t minor = vk::apiVersionMinor(ApiVersion);
        uint32_t patch = vk::apiVersionPatch(ApiVersion);

        VRTR_INFO("Vulkan API Version: {}.{}.{}", major, minor, patch);
        vk::ApplicationInfo appInfo{
            .pApplicationName = "Vulkan Ray tracer with radiosity",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = vk::makeApiVersion(
                 static_cast<uint32_t>(0),
                 major,
                 minor,
                 patch 
                )
        };

        vk::InstanceCreateInfo createInfo
        {
            .pApplicationInfo = &appInfo,
        };

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        // if(enableValidationLayers)
        // {
        //     ValidationLayers::populateDebugMessengerCreateInfo(debugCreateInfo);

        //     vk::InstanceCreateInfo createInfo
        //     {
        //         .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        //         .ppEnabledLayerNames = validationLayers.data(),
        //         // .pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo
        //     };
        // }
        // else
        // {
        //     createInfo.enabledLayerCount = 0;
        //     createInfo.ppEnabledLayerNames = nullptr;
        //     createInfo.pNext = nullptr;
        // }
    
        auto extensions = getRequiredExtensions();
        uint32_t extensionsCount = static_cast<uint32_t>(extensions.size());
        createInfo.enabledExtensionCount = extensionsCount;
        createInfo.ppEnabledExtensionNames = extensions.data();

        uint32_t instanceExtensionsCount = 0;
        auto extensionProperties = context.enumerateInstanceExtensionProperties();
        
        if(!checkExtensionsSupport(extensions.data(), extensionsCount, extensionProperties))
            VRTR_CRITICAL("GLFW EXTENSION DOES NOT MACH INSTANCE EXTENSIONS");

        try
        {
            instance = vk::raii::Instance(context, createInfo);
        }
        catch (const vk::SystemError& e)
        {
            VRTR_CRITICAL("Failed to create Vulkan instance: {}", e.what());
            throw;
        }
        catch (const std::exception& e)
        {
            VRTR_CRITICAL("Failed to create Vulkan instance: {}", e.what());
            throw;
        }
    }
    bool VulkanInstance::checkExtensionsSupport(const char** glfwExtensions, uint32_t glfwExtensionCount, 
        const std::vector<vk::ExtensionProperties>& extensionsProperties)
    {
        for(uint32_t extension = 0;  extension < glfwExtensionCount; extension++)
        {
            if (std::ranges::none_of(extensionsProperties,
                [glfwExtension = glfwExtensions[extension]](auto const& extensionProperty)
                {
                    return (strcmp(glfwExtension, extensionProperty.extensionName) == 0);
                }))
                {
                    throw std::runtime_error("Required extension not supported: " + std::string(glfwExtensions[extension]));
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

        // if(enableValidationLayers)
        // {
        //     extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        // }

        return extensions;
    } 
}
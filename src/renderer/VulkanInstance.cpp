#include "VulkanInstance.hpp"
#include "pch.h"

namespace VRTR
{
    VulkanInstance::VulkanInstance(vk::raii::Context& ctx)
        : context(ctx)
    {}

    void VulkanInstance::createInstance(vk::raii::Instance& instance)
    {
        VRTR_DEBUG("CREATING VULKAN INSTANCE");
        if(glfwVulkanSupported() != GLFW_TRUE)
        {
            VRTR_CRITICAL("GLFW VULKAN NOT SUPPORTED");
            throw std::runtime_error("GLFW VULKAN NOT SUPPORTED");
        }

        vk::ApplicationInfo appInfo{
            .pApplicationName = "Vulkan Ray tracer with radiosity",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = vk::ApiVersion14
        };

        if(enableValidationLayers && !ValidationLayers::checkLayerValidationSupport(context))
        {
            VRTR_CRITICAL("Validation layer not available!");
            throw std::runtime_error("Validation layer not available!");
        }

        auto extensions = getRequiredExtensions();
        uint32_t extensionsCount = static_cast<uint32_t>(extensions.size());

        auto extensionProperties = context.enumerateInstanceExtensionProperties();

        if(!checkExtensionsSupport(extensions.data(), extensionsCount, extensionProperties))
            VRTR_CRITICAL("GLFW EXTENSION DOES NOT MACH INSTANCE EXTENSIONS");

        auto debugCI = ValidationLayers::debugCreateInfo();
        vk::InstanceCreateInfo createInfo{};
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        createInfo.enabledExtensionCount = extensionsCount;
        createInfo.ppEnabledExtensionNames = extensions.data();
        createInfo.pNext = &debugCI;

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

        if(enableValidationLayers)
        {
            extensions.push_back(vk::EXTDebugUtilsExtensionName);
        }

        return extensions;
    } 
}
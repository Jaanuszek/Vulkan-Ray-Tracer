#include "InstanceManager.hpp"
#include "pch.h"

namespace VRTR
{
    VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                   vk::DebugUtilsMessageTypeFlagsEXT type,
                                                   const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                   void *)
    {
        switch (severity)
        {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
            VRTR_VALIDATION_TRACE("Message: {}", pCallbackData->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
            VRTR_VALIDATION_INFO("Message: {}", pCallbackData->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
            VRTR_VALIDATION_WARN("Message: {}", pCallbackData->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            VRTR_VALIDATION_ERROR("Message: {}", pCallbackData->pMessage);
            break;
        default:
            break;
        }

        return VK_FALSE;
    }

    vk::raii::Instance InstanceManager::createInstance(RendererContext& ctx)
    {
        VRTR_DEBUG("CREATING VULKAN INSTANCE");
        if (glfwVulkanSupported() != GLFW_TRUE)
        {
            VRTR_CRITICAL("GLFW VULKAN NOT SUPPORTED");
            throw std::runtime_error("GLFW VULKAN NOT SUPPORTED");
        }

        std::vector<vk::ExtensionProperties> availableExtensionProperties = ctx.context.enumerateInstanceExtensionProperties();

        auto extensions = getRequiredExtensions();
        uint32_t extensionsCount = static_cast<uint32_t>(extensions.size());

        uint32_t instanceVersion = vk::enumerateInstanceVersion();

        vk::ApplicationInfo appInfo{
            .pApplicationName = "Vulkan Ray tracer with radiosity",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_MAKE_VERSION(
                VK_VERSION_MAJOR(instanceVersion),
                VK_VERSION_MINOR(instanceVersion),
                VK_VERSION_PATCH(instanceVersion))};

#ifndef NDEBUG
        bool has_debug_utils = std::ranges::any_of(
            availableExtensionProperties,
            [](auto const &ep)
            { return strcmp(ep.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0; });

        if (enableValidationLayers && !has_debug_utils)
        {
            VRTR_CRITICAL("Validation layer not available!");
            throw std::runtime_error("Validation layer not available!");
        }
#endif

        if (!checkExtensionsSupport(extensions, availableExtensionProperties))
            VRTR_CRITICAL("GLFW EXTENSION DOES NOT MACH INSTANCE EXTENSIONS");

#ifndef NDEBUG
        auto debugCI = populateDebugMessengerCreateInfo();
        vk::InstanceCreateInfo createInfo{};
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        createInfo.enabledExtensionCount = extensionsCount;
        createInfo.ppEnabledExtensionNames = extensions.data();
        createInfo.pNext = &debugCI;
#else
        vk::InstanceCreateInfo createInfo{};
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
        createInfo.enabledExtensionCount = extensionsCount;
        createInfo.ppEnabledExtensionNames = extensions.data();
#endif

        try
        {
            return vk::raii::Instance(ctx.context, createInfo);
        }
        catch (const vk::SystemError &e)
        {
            VRTR_CRITICAL("Failed to create Vulkan instance: {}", e.what());
            throw;
        }
        catch (const std::exception &e)
        {
            VRTR_CRITICAL("Failed to create Vulkan instance: {}", e.what());
            throw;
        }
    }

    std::vector<const char *> InstanceManager::getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    #ifndef NDEBUG
        if (enableValidationLayers)
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    #endif

        return extensions;
    }

    bool InstanceManager::checkExtensionsSupport(const std::vector<const char *> &glfwExtensions,
                                       const std::vector<vk::ExtensionProperties> &extensionsProperties)
    {
        std::ranges::for_each(glfwExtensions,
                              [&extensionsProperties](auto const &glfwExtension)
                              {
                                  if (std::ranges::none_of(extensionsProperties,
                                                           [glfwExtension](auto const &extensionProperty)
                                                           {
                                                               return (strcmp(glfwExtension, extensionProperty.extensionName) == 0);
                                                           }))
                                  {
                                      throw std::runtime_error("Required extension not supported: " + std::string(glfwExtension));
                                  }
                              });
        return true;
    }

    void InstanceManager::initValidationLayers(vk::raii::Context& ctx, vk::raii::Instance& instance, vk::raii::DebugUtilsMessengerEXT& debugMessenger)
    {
        if (!enableValidationLayers)
            return;

        auto layerProperties = ctx.enumerateInstanceLayerProperties();

        bool validationLayersSupported = std::ranges::all_of(
            validationLayers,
            [layerProperties](const char *layerName)
            {
                return std::ranges::any_of(
                    layerProperties,
                    [layerName](auto const &layerProperty)
                    {
                        return (strcmp(layerName, layerProperty.layerName) == 0);
                    });
            });

        if (!validationLayersSupported)
        {
            VRTR_CRITICAL("Validation layers requested, but not available!");
            throw std::runtime_error("Validation layers requested, but not available!");
        }

        auto debugCI = populateDebugMessengerCreateInfo();

        debugMessenger = instance.createDebugUtilsMessengerEXT(debugCI, nullptr);
    }

    vk::DebugUtilsMessengerCreateInfoEXT InstanceManager::populateDebugMessengerCreateInfo()
    {
        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
        vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
        return vk::DebugUtilsMessengerCreateInfoEXT{
            .messageSeverity = severityFlags,
            .messageType = messageTypeFlags,
            .pfnUserCallback = &debugCallback};
    }
}
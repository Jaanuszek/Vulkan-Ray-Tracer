#pragma once
#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"

namespace VRTR
{
#ifndef NDEBUG
    constexpr bool enableValidationLayers = true;
#else
    constexpr bool enableValidationLayers = false;
#endif

    inline static std::vector<const char *> validationLayers{
        "VK_LAYER_KHRONOS_validation"
    };

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                   vk::DebugUtilsMessageTypeFlagsEXT type,
                                                   const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                   void *);

    class InstanceManager
    {
        public:
            static vk::raii::Instance createInstance(RendererContext& ctx);
        private:
            static std::vector<const char*> getRequiredExtensions();
            static bool checkExtensionsSupport(const std::vector<const char*>& glfwExtensions,
                                       const std::vector<vk::ExtensionProperties>& extensionsProperties);
            static void initValidationLayers(vk::raii::Context& ctx, vk::raii::Instance& instance, vk::raii::DebugUtilsMessengerEXT& debugMessenger);
            static vk::DebugUtilsMessengerCreateInfoEXT populateDebugMessengerCreateInfo();
    };
}
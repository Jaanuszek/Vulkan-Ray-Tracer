#pragma once
#include "pch.h"
#include "Logger.hpp"

#ifdef NDEBUG
    constexpr bool enableValidationLayers = false;
#else
    constexpr bool enableValidationLayers = true;
#endif

namespace VRTR
{
    const std::vector<const char*> validationLayers
    {
        "VK_LAYER_KHRONOS_validation"
    };

    class ValidationLayers
    {
        public:
            ValidationLayers() = default;
            ~ValidationLayers() = default;
            void init(VkInstance instance);
            void destroy(VkInstance instance);
            static bool checkLayerValidationSupport();
            static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
            static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
                    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                    void* pUserData);
        private:
            VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
            void setupDebugMessenger(VkInstance instance);
            void destroyDebugMessenger(VkInstance instance);

    };
}
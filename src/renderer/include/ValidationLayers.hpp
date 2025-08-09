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
    inline const std::vector<const char*> validationLayers
    {
        "VK_LAYER_KHRONOS_validation"
    };

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*);

    class ValidationLayers
    {
        public:
            ValidationLayers(vk::raii::Context& ctx);
            ~ValidationLayers() = default;
            void init(vk::raii::Instance& instance);
            static bool checkLayerValidationSupport(vk::raii::Context& ctx);
            static vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo();
        private:
            vk::raii::Context& context;
            vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
            void setupDebugMessenger(vk::raii::Instance&  instance);
    };
}
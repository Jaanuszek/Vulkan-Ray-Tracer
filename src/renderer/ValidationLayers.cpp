#include "ValidationLayers.hpp"
#include "pch.h"

namespace VRTR
{
    VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
            vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
            vk::DebugUtilsMessageTypeFlagsEXT type,
            const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void*)
    {
        switch (severity) {
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

    ValidationLayers::ValidationLayers(vk::raii::Context& ctx)
        : context(ctx)
    {}

    void ValidationLayers::init(vk::raii::Instance&instance)
    {
        setupDebugMessenger(instance);
    }

    bool ValidationLayers::checkLayerValidationSupport(vk::raii::Context& ctx)
    {
        auto layerProperties = ctx.enumerateInstanceLayerProperties();
        for(const char* layerName : validationLayers)
        {
            bool foundLayer = false;

            for(const auto& layerProperty : layerProperties)
            {
                if(strcmp(layerName, layerProperty.layerName) == 0)
                {
                    foundLayer = true;
                    break;
                }
            }

            if(foundLayer == false)
            {
                return false;
            }
        }
        return true;
    }

    vk::DebugUtilsMessengerCreateInfoEXT ValidationLayers::debugCreateInfo()
    {
        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags( vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError );
        vk::DebugUtilsMessageTypeFlagsEXT    messageTypeFlags( vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation );
        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
            .messageSeverity = severityFlags,
            .messageType = messageTypeFlags,
            .pfnUserCallback = &debugCallback
        };
        return debugUtilsMessengerCreateInfoEXT;
    }
    void ValidationLayers::setupDebugMessenger(vk::raii::Instance& instance)
    {
        if(!enableValidationLayers) return;

        auto debugUtilsMessengerCreateInfoEXT = debugCreateInfo();
        debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT, nullptr);
    }

}
// #include "ValidationLayers.hpp"
// #include "pch.h"

// namespace VRTR
// {
//     void ValidationLayers::init(VkInstance instance)
//     {
//         setupDebugMessenger(instance);
//     }

//     void ValidationLayers::destroy(VkInstance instance)
//     {
//         destroyDebugMessenger(instance);
//     }

//     bool ValidationLayers::checkLayerValidationSupport()
//     {
//         uint32_t extensionCount;
//         vkEnumerateInstanceLayerProperties(&extensionCount, nullptr);
//         std::vector<VkLayerProperties> layerProperties(extensionCount);
//         vkEnumerateInstanceLayerProperties(&extensionCount, layerProperties.data());

//         for(const char* layerName : validationLayers)
//         {
//             bool foundLayer = false;

//             for(const auto& layerProperty : layerProperties)
//             {
//                 if(strcmp(layerName, layerProperty.layerName) == 0)
//                 {
//                     foundLayer = true;
//                     break;
//                 }
//             }

//             if(foundLayer == false)
//             {
//                 return false;
//             }
//         }
//         return true;
//     }

//     VKAPI_ATTR VkBool32 VKAPI_CALL ValidationLayers::debugCallback(
//     VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
//     VkDebugUtilsMessageTypeFlagsEXT messageType,
//     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
//     void* pUserData)
//     {
//         // TODO add another logger for validation layers
//         switch (messageSeverity)
//         {
//             case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
//                 VRTR_VALIDATION_TRACE("MessageID: {}, Message: {}", pCallbackData->pMessageIdName, pCallbackData->pMessage);
//                 break;
//             case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
//                 VRTR_VALIDATION_INFO("MessageID: {}, Message: {}", pCallbackData->pMessageIdName, pCallbackData->pMessage);
//                 break;
//             case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
//                 VRTR_VALIDATION_WARN("MessageID: {}, Message: {}", pCallbackData->pMessageIdName, pCallbackData->pMessage);
//                 break;
//             case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
//                 VRTR_VALIDATION_ERROR("MessageID: {}, Message: {}", pCallbackData->pMessageIdName, pCallbackData->pMessage);
//                 break;
//             default:
//                 VRTR_VALIDATION_CRITICAL("MessageID: {}, Message: {}", pCallbackData->pMessageIdName, pCallbackData->pMessage);
//                 return VK_FALSE;
//         }
//         // I will disable it for now
//         bool enableVerbose = false;
//         if(enableVerbose){
//             for (uint32_t i = 0; i < pCallbackData->objectCount; ++i)
//             {
//                 if(pCallbackData->pObjects[i].objectHandle)
//                 {
//                     VRTR_VALIDATION_INFO("Object [{}]: Handle: {}", i, pCallbackData->pObjects[i].objectHandle);
//                 }
//                 if(pCallbackData->pObjects[i].pObjectName)
//                 {
//                     VRTR_VALIDATION_INFO("Object [{}]: Name: {}", i, pCallbackData->pObjects[i].pObjectName);
//                 }
//             }
//         }

//         return VK_FALSE;
//     }
    
//     void ValidationLayers::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
//     {
//         createInfo = {};
//         createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
//         createInfo.pNext = nullptr;
//         createInfo.flags = 0;
//         createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
//                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
//                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
//         createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
//                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
//                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
//         // VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT might be interesting flag to set in the future
//         createInfo.pfnUserCallback = ValidationLayers::debugCallback;
//         createInfo.pUserData = nullptr;
//     }

//     void ValidationLayers::setupDebugMessenger(VkInstance instance)
//     {
//         if(!enableValidationLayers) return;

//         VkDebugUtilsMessengerCreateInfoEXT createInfo{};
//         populateDebugMessengerCreateInfo(createInfo);

//         PFN_vkVoidFunction temp_fp;
//         temp_fp = vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
//         if ( !temp_fp ) throw std::runtime_error("Failed to load vkCreateDebugUtilsMessengerEXT function");

//         auto vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(temp_fp);
//         if (vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
//         {
//             throw std::runtime_error("failed to set up debug messenger!");
//         }
//     }

//     void ValidationLayers::destroyDebugMessenger(VkInstance instance)
//     {
//         if (debugMessenger != VK_NULL_HANDLE)
//         {
//             PFN_vkVoidFunction temp_fp;
//             temp_fp = vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
//             if ( !temp_fp ) throw std::runtime_error("Failed to load vkDestroyDebugUtilsMessengerEXT function");

//             auto vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(temp_fp);
//             vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
//             debugMessenger = VK_NULL_HANDLE;
//         }
//     }
// }
#include "ValidationLayers.hpp"
#include "pch.h"

namespace VRTR
{
    bool ValidationLayers::checkLayerValidationSupport()
    {
        uint32_t extensionCount;
        vkEnumerateInstanceLayerProperties(&extensionCount, nullptr);
        std::vector<VkLayerProperties> layerProperties(extensionCount);
        vkEnumerateInstanceLayerProperties(&extensionCount, layerProperties.data());

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
}
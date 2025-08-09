#include "RTRenderer.hpp"
#include "pch.h"

namespace VRTR
{
    void RTRenderer::init()
    {
        VRTR_DEBUG("RTRENDERER INIT");
        VRTR_Instance = std::make_unique<VulkanInstance>(context);
        VRTR_valLayers = std::make_unique<ValidationLayers>(context);
        VRTR_PhysicalDevice = std::make_unique<PhysicalDevice>();
        VRTR_LogicalDevice = std::make_unique<LogicalDevice>();

        VRTR_Instance->createInstance(instance);
        VRTR_valLayers->setupDebugMessenger(instance);
        VRTR_PhysicalDevice->pickPhysicalDevice(instance, physicalDevice);
        VRTR_LogicalDevice->createLogicalDevice(physicalDevice, logicalDevice, graphicsQueue);
    }
}
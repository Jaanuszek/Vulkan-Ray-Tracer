#include "RTRenderer.hpp"
#include "pch.h"

namespace VRTR
{
    void RTRenderer::init()
    {
        VRTR_DEBUG("RTRENDERER INIT");
        VRTR_Instance = std::make_unique<VulkanInstance>(context);
        VRTR_valLayers = std::make_unique<ValidationLayers>(context);

        VRTR_Instance->createInstance(instance);
        VRTR_valLayers->setupDebugMessenger(instance);
    }
}
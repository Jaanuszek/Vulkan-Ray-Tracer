#include "RTRenderer.hpp"
#include "pch.h"

namespace VRTR
{
    void RTRenderer::init()
    {
        VRTR_DEBUG("RTRENDERER INIT");
        instance = std::make_shared<VulkanInstance>();
        // validationLayers = std::make_shared<ValidationLayers>();

        instance->init();
        // validationLayers->init(instance->getInstance());
    }

    void RTRenderer::destroy()
    {
        VRTR_DEBUG("RTRENDERER DESTROY");
        // validationLayers->destroy(instance->getInstance());
        instance->destroy();
    }
}
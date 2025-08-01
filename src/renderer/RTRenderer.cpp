#include "RTRenderer.hpp"
#include "pch.h"

namespace VRTR
{
    void RTRenderer::init()
    {
        VRTR_DEBUG("RTRENDERER INIT");
        instance = std::make_shared<VulkanInstance>();
    }

    void RTRenderer::destroy()
    {
        VRTR_DEBUG("RTRENDERER DESTROY");
    }
}
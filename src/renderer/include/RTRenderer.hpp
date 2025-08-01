#pragma once

#include "renderer_export.h"
#include "VulkanInstance.hpp"

namespace VRTR
{
    class RENDERER_EXPORT RTRenderer
    {
        public:
            RTRenderer() = default;
            ~RTRenderer() = default;
            void init();
            void destroy();
        private:
            VulkanInstance instance;
    };
}
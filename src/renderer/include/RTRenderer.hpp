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
            // Probably I would not need to store these as shared_ptrs.
            // But I will keep it for now
            std::shared_ptr<VulkanInstance> instance;
            std::shared_ptr<ValidationLayers> validationLayers;
    };
}
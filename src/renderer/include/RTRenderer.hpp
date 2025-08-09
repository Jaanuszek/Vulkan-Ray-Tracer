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
        private:
            vk::raii::Context context;
            vk::raii::Instance instance{nullptr};
            std::unique_ptr<VulkanInstance> VRTR_Instance;
            std::unique_ptr<ValidationLayers> VRTR_valLayers;
    };
}
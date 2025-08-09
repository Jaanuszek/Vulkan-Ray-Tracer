#pragma once

#include "renderer_export.h"
#include "VulkanInstance.hpp"
#include "ValidationLayers.hpp"
#include "PhysicalDevice.hpp"
#include "LogicalDevice.hpp"

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
            vk::raii::PhysicalDevice physicalDevice{nullptr};
            vk::raii::Device logicalDevice{nullptr};
            vk::raii::Queue graphicsQueue{nullptr}; // it's automatically created along with the logical device

            std::unique_ptr<VulkanInstance> VRTR_Instance;
            std::unique_ptr<ValidationLayers> VRTR_valLayers;
            std::unique_ptr<PhysicalDevice> VRTR_PhysicalDevice;
            std::unique_ptr<LogicalDevice> VRTR_LogicalDevice;
    };
}
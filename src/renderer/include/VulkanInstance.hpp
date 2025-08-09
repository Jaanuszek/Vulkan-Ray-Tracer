#pragma once
#include "renderer_export.h"
#include "Logger.hpp"
#include "ValidationLayers.hpp"

namespace VRTR
{
    class RENDERER_EXPORT VulkanInstance
    {
        public:
            VulkanInstance(vk::raii::Context& ctx);
            ~VulkanInstance() = default;
            void createInstance(vk::raii::Instance& inst);
        private:
            vk::raii::Context& context;
            bool checkExtensionsSupport(const char** glfwExtensions, uint32_t glfwExtensionCount,
                const std::vector<vk::ExtensionProperties>& extensionsProperties);
            std::vector<const char*> getRequiredExtensions();
    };
}
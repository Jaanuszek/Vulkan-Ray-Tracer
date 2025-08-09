#pragma once
#include "renderer_export.h"
#include "Logger.hpp"
#include "ValidationLayers.hpp"

namespace VRTR
{
    class RENDERER_EXPORT VulkanInstance
    {
        public:
            VulkanInstance(vk::raii::Context& ctx, vk::raii::Instance& inst);
            ~VulkanInstance() = default;
            void init();
            void createInstance();
            void destroy();
        private:
            vk::raii::Context& context;
            vk::raii::Instance& instance;
            bool checkExtensionsSupport(const char** glfwExtensions, uint32_t glfwExtensionCount,
                const std::vector<vk::ExtensionProperties>& extensionsProperties);
            std::vector<const char*> getRequiredExtensions();
    };
}
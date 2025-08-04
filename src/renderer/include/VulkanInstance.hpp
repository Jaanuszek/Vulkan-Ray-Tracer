#pragma once
#include "renderer_export.h"
#include "Logger.hpp"
#include "ValidationLayers.hpp"

namespace VRTR
{
    class RENDERER_EXPORT VulkanInstance
    {
        public:
            VulkanInstance() = default;
            ~VulkanInstance() = default;
            void init();
            void destroy();
            const vk::raii::Instance& getInstance() const { return instance; }
        private:
            vk::raii::Context context;
            vk::raii::Instance instance{nullptr};
            void createInstance();
            bool checkExtensionsSupport(const char** glfwExtensions, uint32_t glfwExtensionCount,
                const std::vector<vk::ExtensionProperties>& extensionsProperties);
            std::vector<const char*> getRequiredExtensions();
    };
}
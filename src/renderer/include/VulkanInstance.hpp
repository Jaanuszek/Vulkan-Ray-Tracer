#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "renderer_export.h"
#include "Logger.hpp"

namespace VRTR
{
    class RENDERER_EXPORT VulkanInstance
    {
        public:
            VulkanInstance();
            ~VulkanInstance();
            VkInstance getInstance() const { return instance; }
        private:
            VkInstance instance;
            bool checkExtensionsSupport(const char** glfwExtentions, uint32_t glfwExtensionCount,
                const std::vector<VkExtensionProperties>& extensionsProperties);
    };
}
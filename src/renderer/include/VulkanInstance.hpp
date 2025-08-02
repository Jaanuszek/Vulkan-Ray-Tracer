#include "renderer_export.h"
#include "Logger.hpp"
#include "ValidationLayers.hpp"

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
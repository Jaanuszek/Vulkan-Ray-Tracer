#pragma once

#include "renderer_export.h"
#include "SwapChainManager.hpp"
#include "RasterGraphicsPipeline.hpp"
#include "CommandBuffer.hpp"
#include "ConstantsAndStructs.hpp"

namespace VRTR
{
    #ifndef NDEBUG
        constexpr bool enableValidationLayers = true;
        std::vector<const char*> validationLayers
        {
            "VK_LAYER_KHRONOS_validation"
        };
    #elif
        constexpr bool enableValidationLayers = false;
    #endif

    class RENDERER_EXPORT RTRenderer
    {
        public:
            bool framebufferResized = false;

            RTRenderer() = default;
            ~RTRenderer();
            void init(GLFWwindow* window);
            void drawFrame(GLFWwindow* window);

        private:

            inline static std::vector<const char*> deviceExtensions // gpu logical device extensions
            {
                vk::KHRSwapchainExtensionName,
                vk::KHRSpirv14ExtensionName
            };

            std::vector<const char*> getRequiredExtensions();

            // Check if selected extensions are supported by the instance
            bool checkExtensionsSupport(const std::vector<const char*>& glfwExtensions, 
                                         const std::vector<vk::ExtensionProperties>& extensionsProperties);

            void initInstance();

            #ifndef NDEBUG
                void initValidationLayers();
                vk::DebugUtilsMessengerCreateInfoEXT populateDebugMessengerCreateInfo();
            #endif

            bool isDeviceSuitable(const vk::raii::PhysicalDevice& device);

            uint32_t findQueueFamilies();

            void initPhysicalDeviceAndSurface(GLFWwindow* window);

            void initLogicalDevice();

            void initSwapChain(GLFWwindow* window);

            void initPipeline();

            void initCommandBuffer();

            void createSyncObjects();

            void recordCommandBuffer(uint32_t imageIndex);

        private:
            Context ctx;
            std::unique_ptr<SwapChainManager> swapChainManager;
            SurfaceCapabilities surfaceCapabilities;

            std::unique_ptr<SwapChainManager> VRTR_SwapChain;
            std::unique_ptr<RasterGraphicsPipeline> VRTR_RasterGraphicsPipeline;
            std::unique_ptr<CommandBuffer> VRTR_CommandBuffer;
    };


    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                vk::DebugUtilsMessageTypeFlagsEXT type,
                                                const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                void*);
    inline static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<RTRenderer*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }
}
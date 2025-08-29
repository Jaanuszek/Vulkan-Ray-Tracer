#pragma once

#include "renderer_export.h"
#include "PhysicalDevice.hpp"
#include "LogicalDevice.hpp"
#include "WindowSurface.hpp"
#include "SwapChain.hpp"
#include "RasterGraphicsPipeline.hpp"
#include "CommandBuffer.hpp"
#include "Constants.hpp"

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
        struct Context
        {
            vk::raii::Context context;

            vk::raii::Instance instance{nullptr};

            vk::raii::PhysicalDevice gpu{nullptr};

            vk::raii::Device logicalDevice{nullptr};

            vk::raii::Queue queue{nullptr};

            int32_t graphics_queue_index = -1;

            vk::raii::SurfaceKHR surface{nullptr};

            vk::raii::SwapchainKHR swapChain{nullptr};
            
            std::vector<vk::Image> swapChainImages;

            std::vector<vk::raii::ImageView> swapChainImageViews;

            vk::raii::PipelineLayout pipelineLayout{nullptr};

            vk::raii::Pipeline rasterGraphicsPipeline{nullptr};

            vk::raii::CommandPool commandPool{nullptr};

            std::vector<vk::raii::CommandBuffer> commandBuffers;

            vk::DebugUtilsMessengerEXT debugMessenger{nullptr};

            vk::Buffer vertex_buffer{nullptr};

            // SYNC VARIABLES
            std::vector<vk::raii::Semaphore> presentCompleteSemaphores;

            std::vector<vk::raii::Semaphore> renderCompleteSemaphores;

            std::vector<vk::raii::Fence> drawFences;
        };

        public:
            bool framebufferResized = false;

            RTRenderer() = default;
            // ~RTRenderer();
            void init(GLFWwindow* window);
            void createSyncObjects();
            void recordCommandBuffer(uint32_t imageIndex);
            // void drawFrame();

        private:

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
            void initSwapChain();
            void initPipeline();

        private:
            Context ctx;
            // GENERAL VARIABLES
            // GLFWwindow* window = nullptr;
            // vk::raii::Context context;
            // vk::raii::Instance instance{nullptr};
            // vk::raii::PhysicalDevice physicalDevice{nullptr};
            // vk::raii::Device logicalDevice{nullptr};
            // vk::raii::Queue Queue{nullptr}; // it's automatically created along with the logical device
            // vk::raii::SurfaceKHR surface{nullptr};
            // vk::raii::SwapchainKHR swapChain{nullptr};
            // std::vector<vk::Image> swapChainImages;
            // std::vector<vk::raii::ImageView> swapChainImageViews;
            // vk::raii::PipelineLayout pipelineLayout{nullptr};
            // vk::raii::Pipeline rasterGraphicsPipeline{nullptr};
            // vk::raii::CommandPool commandPool{nullptr};
            // std::vector<vk::raii::CommandBuffer> commandBuffers;

            // // SYNC VARIABLES
            // std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
            // std::vector<vk::raii::Semaphore> renderCompleteSemaphores;
            // std::vector<vk::raii::Fence> drawFences;

            // std::unique_ptr<VulkanInstance> VRTR_Instance;
            // std::unique_ptr<ValidationLayers> VRTR_valLayers;
            // std::unique_ptr<PhysicalDevice> VRTR_PhysicalDevice;
            // std::unique_ptr<LogicalDevice> VRTR_LogicalDevice;
            // std::unique_ptr<WindowSurface> VRTR_WindowSurface;
            // std::unique_ptr<SwapChain> VRTR_SwapChain;
            // std::unique_ptr<RasterGraphicsPipeline> VRTR_RasterGraphicsPipeline;
            // std::unique_ptr<CommandBuffer> VRTR_CommandBuffer;

            // SurfaceCapabilities surfaceCapabilities;
            // uint32_t QueueFamilyIndex; // graphics and presentation queue
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
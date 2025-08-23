#pragma once

#include "renderer_export.h"
#include "VulkanInstance.hpp"
#include "ValidationLayers.hpp"
#include "PhysicalDevice.hpp"
#include "LogicalDevice.hpp"
#include "WindowSurface.hpp"
#include "SwapChain.hpp"
#include "RasterGraphicsPipeline.hpp"
#include "CommandBuffer.hpp"

namespace VRTR
{
    class RENDERER_EXPORT RTRenderer
    {
        public:
            RTRenderer() = default;
            ~RTRenderer();
            void init(GLFWwindow* window);
            void createSyncObjects();
            void recordCommandBuffer(uint32_t imageIndex);
            void drawFrame();
        private:
            // GENERAL VARIABLES
            GLFWwindow* window = nullptr;
            vk::raii::Context context;
            vk::raii::Instance instance{nullptr};
            vk::raii::PhysicalDevice physicalDevice{nullptr};
            vk::raii::Device logicalDevice{nullptr};
            vk::raii::Queue Queue{nullptr}; // it's automatically created along with the logical device
            vk::raii::SurfaceKHR surface{nullptr};
            vk::raii::SwapchainKHR swapChain{nullptr};
            std::vector<vk::Image> swapChainImages;
            std::vector<vk::raii::ImageView> swapChainImageViews;
            vk::raii::PipelineLayout pipelineLayout{nullptr};
            vk::raii::Pipeline rasterGraphicsPipeline{nullptr};
            vk::raii::CommandPool commandPool{nullptr};
            vk::raii::CommandBuffer commandBuffer{nullptr};

            // SYNC VARIABLES
            vk::raii::Semaphore presentCompleteSemaphore{nullptr}; // image has been acquired and is ready for rendering
            vk::raii::Semaphore renderCompleteSemaphore{nullptr}; // rendering finished and presentation can happen
            vk::raii::Fence drawFence{nullptr}; 

            std::unique_ptr<VulkanInstance> VRTR_Instance;
            std::unique_ptr<ValidationLayers> VRTR_valLayers;
            std::unique_ptr<PhysicalDevice> VRTR_PhysicalDevice;
            std::unique_ptr<LogicalDevice> VRTR_LogicalDevice;
            std::unique_ptr<WindowSurface> VRTR_WindowSurface;
            std::unique_ptr<SwapChain> VRTR_SwapChain;
            std::unique_ptr<RasterGraphicsPipeline> VRTR_RasterGraphicsPipeline;
            std::unique_ptr<CommandBuffer> VRTR_CommandBuffer;

            SurfaceCapabilities surfaceCapabilities;
            uint32_t QueueFamilyIndex; // graphics and presentation queue
    };
}
#pragma once

#include "renderer_export.h"
#include "VulkanInstance.hpp"
#include "ValidationLayers.hpp"
#include "PhysicalDevice.hpp"
#include "LogicalDevice.hpp"
#include "WindowSurface.hpp"
#include "SwapChain.hpp"

namespace VRTR
{
    class RENDERER_EXPORT RTRenderer
    {
        public:
            RTRenderer() = default;
            ~RTRenderer() = default;
            void init(GLFWwindow* window);
        private:
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

            std::unique_ptr<VulkanInstance> VRTR_Instance;
            std::unique_ptr<ValidationLayers> VRTR_valLayers;
            std::unique_ptr<PhysicalDevice> VRTR_PhysicalDevice;
            std::unique_ptr<LogicalDevice> VRTR_LogicalDevice;
            std::unique_ptr<WindowSurface> VRTR_WindowSurface;
            std::unique_ptr<SwapChain> VRTR_SwapChain;
    };
}
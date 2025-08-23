#pragma once

#include "Logger.hpp"

namespace VRTR
{
    struct SurfaceCapabilities
    {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> availableFormats;
        std::vector<vk::PresentModeKHR> availablePresentModes;
        vk::SurfaceFormatKHR surfaceFormat;
        vk::PresentModeKHR presentMode;
        vk::Extent2D extent;
    };

    class SwapChain
    {
        public:
            SwapChain(vk::raii::Device& device, vk::raii::SurfaceKHR& surface, vk::raii::SwapchainKHR& swapChain,
                      std::vector<vk::Image>& swapChainImages, std::vector<vk::raii::ImageView>& imageViews);
            ~SwapChain() = default;
                
            inline SurfaceCapabilities getSurfaceCapabilities() { return surfaceCapabilities; }

            void createSwapChain(vk::raii::PhysicalDevice physicalDevice, GLFWwindow* window);
            void createImageViews();
            void cleanupSwapChain();
            void recreateSwapChain(vk::raii::PhysicalDevice physicalDevice, GLFWwindow* window);
        private:
        vk::raii::Device& device;
        vk::raii::SurfaceKHR& surface;
        vk::raii::SwapchainKHR& swapChain;
        std::vector<vk::Image>& swapChainImages;
        std::vector<vk::raii::ImageView>& swapChainImageViews;
        SurfaceCapabilities surfaceCapabilities;

        SurfaceCapabilities generateSurfaceCapabilities(vk::raii::PhysicalDevice& physicalDevice,
                                                                vk::raii::SurfaceKHR& surface,
                                                                GLFWwindow* window);

        vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);

        vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);

        vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
    };
}
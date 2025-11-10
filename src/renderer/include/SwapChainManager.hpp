#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"

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

    class SwapChainManager
    {
        public:
            SwapChainManager(VULKAN_CONTEXT& ctx);
            ~SwapChainManager() = default;

            inline SurfaceCapabilities getSurfaceCapabilities() { return surfaceCapabilities; }

            void createSwapChain(GLFWwindow* window);
            void createImageViews();
            void cleanupSwapChain();
            void recreateSwapChain(GLFWwindow* window, int& w, int& h);
        private:            
            SurfaceCapabilities surfaceCapabilities;

            SurfaceCapabilities generateSurfaceCapabilities(GLFWwindow* window);

            vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);

            vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);

            vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
        
        private:
            VULKAN_CONTEXT& ctx;
    };
}
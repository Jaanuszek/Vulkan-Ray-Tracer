#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"

namespace VRTR
{

    class SwapChainManager
    {
        public:
            SwapChainManager(vk::raii::Device &device, vk::raii::PhysicalDevice &gpu, vk::raii::SurfaceKHR &surface);

            void init(GLFWwindow *window);

            void recreateSwapChain(GLFWwindow* window, int& w, int& h);

            inline vk::Image getSwapChainImage(size_t index) const
            {
                return swapChainImages.at(index);
            }

            inline std::vector<vk::Image>& getSwapChainImages()
            {
                return swapChainImages;
            }
            inline vk::raii::SwapchainKHR& getSwapChain()
            {
                return swapChain;
            }

        private:            

            void createSwapChain(GLFWwindow* window);

            void createImageViews();

            void cleanupSwapChain();

            vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);

            vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);

            vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
        
        private:
            vk::raii::Device &device;
            vk::raii::PhysicalDevice &gpu;
            vk::raii::SurfaceKHR &surface;
            vk::raii::SwapchainKHR swapChain{nullptr};
            std::vector<vk::Image> swapChainImages;
            std::vector<vk::raii::ImageView> swapChainImageViews;
            vk::Format imageFormat;
            vk::SurfaceFormatKHR surfaceFormat;
            vk::PresentModeKHR presentMode;
            vk::Extent2D extent;
    };
}
#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"

namespace VRTR
{

    class SwapChainManager
    {
        public:
            SwapChainManager(RendererContext& ctx);

            void init(GLFWwindow *window);

            void recreateSwapChain(GLFWwindow* window, int& w, int& h);

            inline vk::Image getSwapChainImage(size_t index) const
            {
                return swapChainImages.at(index);
            }

            inline vk::ImageView getSwapChainImageView(size_t index) const
            {
                return *swapChainImageViews.at(index);
            }

            inline vk::Format getImageFormat() const
            {
                return imageFormat;
            }

            inline vk::Extent2D getExtent() const
            {
                return extent;
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
            RendererContext &ctx;
            vk::raii::SwapchainKHR swapChain{nullptr};
            std::vector<vk::Image> swapChainImages;
            std::vector<vk::raii::ImageView> swapChainImageViews;
            vk::Format imageFormat;
            vk::SurfaceFormatKHR surfaceFormat;
            vk::PresentModeKHR presentMode;
            vk::Extent2D extent;
    };
}
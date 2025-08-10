#pragma once

#include "Logger.hpp"
#include "WindowSurface.hpp"

namespace VRTR
{
    class SwapChain
    {
        public:
            SwapChain(VRTR::SurfaceCapabilities surCap);
            ~SwapChain() = default;

            void createSwapChain(vk::raii::Device& device, vk::raii::SurfaceKHR& surface,
                                 vk::raii::SwapchainKHR& swapChain, 
                                 std::vector<vk::Image>& swapChainImages);
        private:
        SurfaceCapabilities surfaceCapabilities;
    };
}
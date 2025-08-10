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

    class WindowSurface
    {
        public:
            WindowSurface() = default;
            ~WindowSurface() = default;

            void setupSurface(vk::raii::Instance& inst, GLFWwindow* window, vk::raii::SurfaceKHR& sur);

            SurfaceCapabilities generateSurfaceCapabilities(vk::raii::PhysicalDevice& physicalDevice,
                                                            vk::raii::SurfaceKHR& surface,
                                                            GLFWwindow* window);

        private:
            vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);

            vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);

            vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
    };
}
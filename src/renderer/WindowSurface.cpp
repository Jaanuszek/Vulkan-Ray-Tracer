#include "pch.h"
#include "WindowSurface.hpp"

namespace VRTR
{
    void WindowSurface::setupSurface(vk::raii::Instance& inst, GLFWwindow* window, vk::raii::SurfaceKHR& sur)
    {
        VRTR_DEBUG("INITIALIZING WINDOW SURFACE");

        VkSurfaceKHR tempSurface;
        if(glfwCreateWindowSurface(*inst, window, nullptr, &tempSurface) != VK_SUCCESS)
        {
            VRTR_ERROR("Failed to create window surface");
            throw std::runtime_error("Failed to create window surface");
        }
        sur = vk::raii::SurfaceKHR(inst, tempSurface);
    }

    SurfaceCapabilities WindowSurface::generateSurfaceCapabilities(vk::raii::PhysicalDevice& physicalDevice,
                                                                   vk::raii::SurfaceKHR& surface,
                                                                   GLFWwindow* window)
    {
        VRTR_DEBUG("GENERATING SURFACE CAPABILITIES");

        SurfaceCapabilities capabilities;
        auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
        auto availableFormats = physicalDevice.getSurfaceFormatsKHR(surface);
        auto availablePresentModes = physicalDevice.getSurfacePresentModesKHR(surface);

        capabilities.capabilities = surfaceCapabilities;
        capabilities.availableFormats = availableFormats;
        capabilities.availablePresentModes = availablePresentModes;
        capabilities.surfaceFormat = chooseSurfaceFormat(availableFormats);
        capabilities.presentMode = choosePresentMode(availablePresentModes);
        capabilities.extent = chooseSwapExtent(surfaceCapabilities, window);

        return capabilities;
    }
                                                    
    vk::SurfaceFormatKHR WindowSurface::chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
    {
        for(const auto& format : availableFormats)
        {
            if(format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                VRTR_DEBUG("Using B8G8R8A8_SRGB format with SRGB nonlinear color space");
                return format;
            }
        }

        return availableFormats[0];
    }

    vk::PresentModeKHR WindowSurface::choosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
    {
        for(const auto& presentMode : availablePresentModes)
        {
            if(presentMode == vk::PresentModeKHR::eMailbox)
            {
                VRTR_DEBUG("Using Mailbox present mode");
                return presentMode; // Prefer mailbox for lower latency
            }
        }

        return vk::PresentModeKHR::eFifo; // Fallback to FIFO
    }

    vk::Extent2D WindowSurface::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
    {
        // If currentExtent is not set to special uint32_t max value,
        // then we need to use it
        if(capabilities.currentExtent.width != UINT32_MAX)
        {
            return capabilities.currentExtent;
        }
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        return vk::Extent2D{ 
            std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width), 
            std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height) 
        };
    }
}

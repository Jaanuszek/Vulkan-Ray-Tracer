#include "pch.h"
#include "SwapChain.hpp"

namespace VRTR
{
    SwapChain::SwapChain(vk::raii::Device& device, vk::raii::SurfaceKHR& surface, vk::raii::SwapchainKHR& swapChain,
                      std::vector<vk::Image>& swapChainImages, std::vector<vk::raii::ImageView>& imageViews)
        :  device(device), surface(surface),
          swapChain(swapChain), swapChainImages(swapChainImages), swapChainImageViews(imageViews)
    {}

    SurfaceCapabilities SwapChain::generateSurfaceCapabilities(vk::raii::PhysicalDevice& physicalDevice,
                                                                   vk::raii::SurfaceKHR& surface,
                                                                   GLFWwindow* window)
    {
        // VRTR_DEBUG("GENERATING SURFACE CAPABILITIES");

        SurfaceCapabilities capabilities;
        auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
        auto availableFormats = physicalDevice.getSurfaceFormatsKHR(surface);
        auto availablePresentModes = physicalDevice.getSurfacePresentModesKHR(surface);

        capabilities.capabilities = surfaceCapabilities;
        capabilities.availableFormats = availableFormats;
        capabilities.availablePresentModes = availablePresentModes;
        capabilities.surfaceFormat = SwapChain::chooseSurfaceFormat(availableFormats);
        capabilities.presentMode = SwapChain::choosePresentMode(availablePresentModes);
        capabilities.extent = SwapChain::chooseSwapExtent(surfaceCapabilities, window);

        return capabilities;
    }

    vk::SurfaceFormatKHR SwapChain::chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
    {
        for(const auto& format : availableFormats)
        {
            if(format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                // VRTR_DEBUG("Using B8G8R8A8_SRGB format with SRGB nonlinear color space");
                return format;
            }
        }

        return availableFormats[0];
    }

    vk::PresentModeKHR SwapChain::choosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
    {
        for(const auto& presentMode : availablePresentModes)
        {
            if(presentMode == vk::PresentModeKHR::eMailbox)
            {
                // VRTR_DEBUG("Using Mailbox present mode");
                return presentMode; // Prefer mailbox for lower latency
            }
        }

        return vk::PresentModeKHR::eFifo; // Fallback to FIFO
    }

    vk::Extent2D SwapChain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
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


    void SwapChain::createSwapChain(vk::raii::PhysicalDevice physicalDevice, GLFWwindow* window)
    {
        surfaceCapabilities = generateSurfaceCapabilities(physicalDevice, surface, window);
        vk::SurfaceFormatKHR  surfaceFormat = surfaceCapabilities.surfaceFormat;
        vk::PresentModeKHR presentMode = surfaceCapabilities.presentMode;
        vk::Extent2D extent = surfaceCapabilities.extent;

        vk::SurfaceCapabilitiesKHR VK_capabilities = surfaceCapabilities.capabilities;

        auto minImageCount = std::max( 2u, VK_capabilities.minImageCount);

        // if maxImageCount is 0 it means there is no limit
        minImageCount = (VK_capabilities.maxImageCount > 0
                        && minImageCount > VK_capabilities.maxImageCount)
                        ? VK_capabilities.maxImageCount : minImageCount;

        vk::SwapchainCreateInfoKHR createInfo{
            .flags = vk::SwapchainCreateFlagsKHR{},
            .surface = surface,
            .minImageCount = minImageCount,
            .imageFormat = surfaceFormat.format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = vk::SharingMode::eExclusive,
            .preTransform = VK_capabilities.currentTransform,
            .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode = presentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = nullptr
        };

        // (!!!!!!!!!!!!!!!!!!!)
        // If I ever need to support multiple queue families, I will need to set the imageSharingMode to eConcurrent

        // uint32_t queueFamilyIndices[] = {graphicsFamily, presentFamily};

        // if (graphicsFamily != presentFamily) {
        //     swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        //     swapChainCreateInfo.queueFamilyIndexCount = 2;
        //     swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
        // } else {
        //     swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
        //     swapChainCreateInfo.queueFamilyIndexCount = 0; // Optional
        //     swapChainCreateInfo.pQueueFamilyIndices = nullptr; // Optional
        // }

        swapChain = vk::raii::SwapchainKHR(device, createInfo);
        swapChainImages = swapChain.getImages();
    }

    void SwapChain::createImageViews()
    {
        swapChainImageViews.clear();
        swapChainImageViews.reserve(swapChainImages.size());

        auto format = surfaceCapabilities.surfaceFormat.format;

        vk::ImageViewCreateInfo createInfo
        {
            .pNext = nullptr,
            .flags = {},
            .image = {},
            .viewType = vk::ImageViewType::e2D,
            .format = format,
            .components = {
                // Identity swizzle - default color components
                .r = vk::ComponentSwizzle::eIdentity,
                .g = vk::ComponentSwizzle::eIdentity,
                .b = vk::ComponentSwizzle::eIdentity,
                .a = vk::ComponentSwizzle::eIdentity
            },
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        for (const auto& image : swapChainImages)
        {
            createInfo.image = image;
            // constructing ImageView from vk::raii::ImageView
            swapChainImageViews.emplace_back(device, createInfo);
        }
    }

    void SwapChain::cleanupSwapChain()
    {
        swapChainImageViews.clear();
        swapChain = nullptr;
    }

    void SwapChain::recreateSwapChain(vk::raii::PhysicalDevice physicalDevice, GLFWwindow* window)
    {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        device.waitIdle();

        cleanupSwapChain();

        createSwapChain(physicalDevice, window);
        createImageViews();
    }
}
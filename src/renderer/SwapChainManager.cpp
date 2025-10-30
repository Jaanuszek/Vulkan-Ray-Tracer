#include "pch.h"
#include "SwapChainManager.hpp"

namespace VRTR
{
    SwapChainManager::SwapChainManager(VULKAN_CONTEXT& ctx) : ctx(ctx)
    {

    }

    SurfaceCapabilities SwapChainManager::generateSurfaceCapabilities(GLFWwindow* window)
    {
        // VRTR_DEBUG("GENERATING SURFACE CAPABILITIES");

        SurfaceCapabilities capabilities;
        auto surfaceCapabilities = ctx.gpu.getSurfaceCapabilitiesKHR(ctx.surface);
        auto availableFormats = ctx.gpu.getSurfaceFormatsKHR(ctx.surface);
        auto availablePresentModes = ctx.gpu.getSurfacePresentModesKHR(ctx.surface);

        capabilities.capabilities = surfaceCapabilities;
        capabilities.availableFormats = availableFormats;
        capabilities.availablePresentModes = availablePresentModes;
        capabilities.surfaceFormat = SwapChainManager::chooseSurfaceFormat(availableFormats);
        capabilities.presentMode = SwapChainManager::choosePresentMode(availablePresentModes);
        capabilities.extent = SwapChainManager::chooseSwapExtent(surfaceCapabilities, window);

        return capabilities;
    }

    vk::SurfaceFormatKHR SwapChainManager::chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
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

    vk::PresentModeKHR SwapChainManager::choosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
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

    vk::Extent2D SwapChainManager::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
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


    void SwapChainManager::createSwapChain(GLFWwindow* window)
    {
        VRTR_DEBUG("CREATING SWAP CHAIN");
        surfaceCapabilities = generateSurfaceCapabilities(window);
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
            .surface = ctx.surface,
            .minImageCount = minImageCount,
            .imageFormat = surfaceFormat.format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst, // TransferSRC - ustawienie bitu ktory mowi ze mozemy zmienic layout danego vk::Image na VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
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

        ctx.swapChain = vk::raii::SwapchainKHR(ctx.logicalDevice, createInfo);
        ctx.swapChainImages = ctx.swapChain.getImages();
    }

    void SwapChainManager::createImageViews()
    {
        VRTR_DEBUG("CREATING IMAGE VIEWS");
        ctx.swapChainImageViews.clear();
        ctx.swapChainImageViews.reserve(ctx.swapChainImages.size());

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
        for (const auto& image : ctx.swapChainImages)
        {
            createInfo.image = image;
            // constructing ImageView from vk::raii::ImageView
            ctx.swapChainImageViews.emplace_back(ctx.logicalDevice, createInfo);
        }
    }

    void SwapChainManager::cleanupSwapChain()
    {
        ctx.swapChainImageViews.clear();
        ctx.swapChain = nullptr;
    }

    void SwapChainManager::recreateSwapChain(GLFWwindow* window, int& w, int& h)
    {
        ctx.logicalDevice.waitIdle();

        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        w = width;
        h = height;

        cleanupSwapChain();

        createSwapChain(window);
        createImageViews();
    }
}
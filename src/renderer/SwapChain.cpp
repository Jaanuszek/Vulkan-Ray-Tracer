#include "pch.h"
#include "SwapChain.hpp"

namespace VRTR
{
    SwapChain::SwapChain(VRTR::SurfaceCapabilities surCap)
        : surfaceCapabilities(surCap)
    {}

    void SwapChain::createSwapChain(vk::raii::Device& device, vk::raii::SurfaceKHR& surface,
                                    vk::raii::SwapchainKHR& swapChain,
                                    std::vector<vk::Image>& swapChainImages)
    {
        VRTR_DEBUG("CREATING SWAP CHAIN");

        vk::SurfaceFormatKHR  surfaceFormat = surfaceCapabilities.surfaceFormat;
        vk::PresentModeKHR presentMode = surfaceCapabilities.presentMode;
        vk::Extent2D extent = surfaceCapabilities.extent;

        vk::SurfaceCapabilitiesKHR VK_capabilities = surfaceCapabilities.capabilities;

        auto minImageCount = std::max( 3u, VK_capabilities.minImageCount);

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
}
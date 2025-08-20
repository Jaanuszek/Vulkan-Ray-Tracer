#include "RTRenderer.hpp"
#include "pch.h"

namespace VRTR
{
    void RTRenderer::init(GLFWwindow* window)
    {
        VRTR_DEBUG("RTRENDERER INIT");
        this->window = window;
        VRTR_Instance = std::make_unique<VulkanInstance>(context);
        VRTR_valLayers = std::make_unique<ValidationLayers>(context);
        VRTR_PhysicalDevice = std::make_unique<PhysicalDevice>();
        VRTR_LogicalDevice = std::make_unique<LogicalDevice>();
        VRTR_WindowSurface = std::make_unique<WindowSurface>();
        VRTR_RasterGraphicsPipeline = std::make_unique<RasterGraphicsPipeline>();
        VRTR_CommandBuffer = std::make_unique<CommandBuffer>(commandPool, commandBuffer);


        VRTR_Instance->createInstance(instance);
        VRTR_valLayers->setupDebugMessenger(instance);

        VRTR_WindowSurface->setupSurface(instance, window, surface);

        VRTR_PhysicalDevice->pickPhysicalDevice(instance, physicalDevice);

        // It has to be done after the physical device is picked
        surfaceCapabilities = VRTR_WindowSurface->generateSurfaceCapabilities(
            physicalDevice, surface, window
        );
        VRTR_SwapChain = std::make_unique<SwapChain>(surfaceCapabilities);
        
        QueueFamilyIndex = PhysicalDevice::findQueueFamilies(physicalDevice, surface);

        VRTR_LogicalDevice->createLogicalDevice(physicalDevice, logicalDevice, Queue, QueueFamilyIndex);
        VRTR_SwapChain->createSwapChain(logicalDevice, surface, swapChain, swapChainImages);
        VRTR_SwapChain->createImageViews(logicalDevice, swapChainImages, swapChainImageViews);
        VRTR_RasterGraphicsPipeline->createPipeline(logicalDevice, surfaceCapabilities,
                                                    pipelineLayout, rasterGraphicsPipeline);
        VRTR_CommandBuffer->createCommandPool(logicalDevice, QueueFamilyIndex);
        VRTR_CommandBuffer->createCommandBuffer(logicalDevice);
    }

    void RTRenderer::renderFrame(uint32_t imageIndex)
    {
        VRTR_CommandBuffer->transition_image_layout
        (
            swapChainImages, imageIndex,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
            vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eColorAttachmentOutput
        ); 

        vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
        vk::RenderingAttachmentInfo attachmentInfo = 
        {
            .imageView = swapChainImageViews.at(imageIndex),
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .resolveMode = vk::ResolveModeFlagBits::eNone,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clearColor
        };

        vk::RenderingInfo renderingInfo = 
        {
            .flags = {},
            .renderArea = {.offset = {0, 0}, .extent = surfaceCapabilities.extent},
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachmentInfo,
            .pDepthAttachment = nullptr,
            .pStencilAttachment = nullptr
        };

        commandBuffer.beginRendering(renderingInfo);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rasterGraphicsPipeline);

        // Setting dynamic states
        commandBuffer.setViewport(0, vk::Viewport{0.0f, 0.0f, 
            static_cast<float>(surfaceCapabilities.extent.width), 
            static_cast<float>(surfaceCapabilities.extent.height), 0.0f, 1.0f});
        commandBuffer.setScissor(0, vk::Rect2D{{0, 0}, surfaceCapabilities.extent});
        commandBuffer.draw(3, 1, 0, 0);

        commandBuffer.endRendering();

        VRTR_CommandBuffer->transition_image_layout(
            swapChainImages, imageIndex,
            vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite, vk::AccessFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eNone
        );

        commandBuffer.end();
    }
}
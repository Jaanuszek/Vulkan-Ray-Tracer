#include "RTRenderer.hpp"
#include "pch.h"

namespace VRTR
{
    RTRenderer::~RTRenderer()
    {
        logicalDevice.waitIdle();
    }

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
        VRTR_CommandBuffer = std::make_unique<CommandBuffer>(commandPool, commandBuffers);


        VRTR_Instance->createInstance(instance);
        VRTR_valLayers->setupDebugMessenger(instance);

        VRTR_WindowSurface->setupSurface(instance, window, surface);

        VRTR_PhysicalDevice->pickPhysicalDevice(instance, physicalDevice);

        VRTR_SwapChain = std::make_unique<SwapChain>(logicalDevice,
                                                    surface, swapChain,
                                                    swapChainImages, swapChainImageViews
                                                    );


        QueueFamilyIndex = PhysicalDevice::findQueueFamilies(physicalDevice, surface);

        VRTR_LogicalDevice->createLogicalDevice(physicalDevice, logicalDevice, Queue, QueueFamilyIndex);
        VRTR_SwapChain->createSwapChain(physicalDevice, window);
        VRTR_SwapChain->createImageViews();
        surfaceCapabilities = VRTR_SwapChain->getSurfaceCapabilities();

        VRTR_RasterGraphicsPipeline->createPipeline(logicalDevice, surfaceCapabilities,
                                                    pipelineLayout, rasterGraphicsPipeline);
        VRTR_CommandBuffer->createCommandPool(logicalDevice, QueueFamilyIndex);
        VRTR_CommandBuffer->createCommandBuffers(logicalDevice);
        createSyncObjects();
    }

    void RTRenderer::createSyncObjects()
    {
        VRTR_DEBUG("Creating Sync Objects");

        presentCompleteSemaphores.clear();
        renderCompleteSemaphores.clear();
        drawFences.clear();

        vk::SemaphoreCreateInfo semaphoreInfo
        {
            .pNext = nullptr,
            .flags = {}
        };

        vk::FenceCreateInfo fenceInfo
        {
            .pNext = nullptr,
            .flags = vk::FenceCreateFlagBits::eSignaled // so we don't wait forever the first time
        };
        for(uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            presentCompleteSemaphores.emplace_back(vk::raii::Semaphore(logicalDevice, semaphoreInfo));
            renderCompleteSemaphores.emplace_back(vk::raii::Semaphore(logicalDevice, semaphoreInfo));
            drawFences.emplace_back(vk::raii::Fence(logicalDevice, fenceInfo));
        }
    }

    void RTRenderer::recordCommandBuffer(uint32_t imageIndex)
    {
        // TODO think about updating only necessary things inside surfaceCapabilities
        // I think it will be only window size and extent???
        surfaceCapabilities = VRTR_SwapChain->getSurfaceCapabilities();
        commandBuffers.at(currentFrame).begin({});
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
            .imageView = swapChainImageViews.at(currentFrame),
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

        commandBuffers.at(currentFrame).beginRendering(renderingInfo);
        commandBuffers.at(currentFrame).bindPipeline(vk::PipelineBindPoint::eGraphics, rasterGraphicsPipeline);

        // Setting dynamic states
        commandBuffers.at(currentFrame).setViewport(0, vk::Viewport{0.0f, 0.0f,
            static_cast<float>(surfaceCapabilities.extent.width),
            static_cast<float>(surfaceCapabilities.extent.height), 0.0f, 1.0f});
        commandBuffers.at(currentFrame).setScissor(0, vk::Rect2D{{0, 0}, surfaceCapabilities.extent});
        commandBuffers.at(currentFrame).draw(3, 1, 0, 0);

        commandBuffers.at(currentFrame).endRendering();

        VRTR_CommandBuffer->transition_image_layout(
            swapChainImages, imageIndex,
            vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite, vk::AccessFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eNone
        );

        commandBuffers.at(currentFrame).end();
    }

    void RTRenderer::drawFrame()
    {
        while (vk::Result::eTimeout == logicalDevice.waitForFences(*drawFences.at(currentFrame), VK_TRUE, UINT64_MAX))
        ;

        // Unfortunately it needs to be inside try catch block, because "acquireNextImage" is throwing exceptions
        // I cant disable it, because i am using vk::raii and it requires exceptions to be enabled :(
        try
        {
            auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, presentCompleteSemaphores.at(semaphoreIndex), nullptr);

        recordCommandBuffer(imageIndex);
        logicalDevice.resetFences({drawFences[currentFrame]});

        vk::PipelineStageFlags waitDestinationStageMask( vk::PipelineStageFlagBits::eColorAttachmentOutput );
        const vk::SubmitInfo submitInfo
        {
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*presentCompleteSemaphores.at(semaphoreIndex),
            .pWaitDstStageMask = &waitDestinationStageMask,
            .commandBufferCount = 1,
            .pCommandBuffers = &*commandBuffers.at(currentFrame),
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &*renderCompleteSemaphores.at(currentFrame)
        };
        Queue.submit({submitInfo}, *drawFences.at(currentFrame));
        const vk::PresentInfoKHR presentInfoKHR{
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*renderCompleteSemaphores.at(currentFrame),
            .swapchainCount = 1,
            .pSwapchains = &*swapChain,
            .pImageIndices = &imageIndex,
            .pResults = nullptr
        };

        result = Queue.presentKHR(presentInfoKHR);

        VRTR::semaphoreIndex = (VRTR::semaphoreIndex + 1) % presentCompleteSemaphores.size();
        VRTR::currentFrame = (VRTR::currentFrame + 1) % VRTR::MAX_FRAMES_IN_FLIGHT;

        }
        catch (const vk::OutOfDateKHRError& e)
        {
            VRTR_SwapChain->recreateSwapChain(physicalDevice, window);
            return;
        }
        catch (const std::exception& e)
        {
            VRTR_CRITICAL("Failed to acquire swap chain image!");
            throw std::runtime_error("Failed to acquire swap chain image!");
        }
    }
}
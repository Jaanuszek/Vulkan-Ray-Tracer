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

        VRTR_Instance->createInstance(instance);
        VRTR_valLayers->setupDebugMessenger(instance);
        VRTR_WindowSurface->setupSurface(instance, window, surface);
        VRTR_PhysicalDevice->pickPhysicalDevice(instance, physicalDevice);

        // It has to be done after the physical device is picked
        VRTR_SwapChain = std::make_unique<SwapChain>(
            VRTR_WindowSurface->generateSurfaceCapabilities(physicalDevice, surface, window)
        );

        VRTR_LogicalDevice->createLogicalDevice(physicalDevice, logicalDevice, Queue, surface);
        VRTR_SwapChain->createSwapChain(logicalDevice, surface, swapChain, swapChainImages);
    }
}
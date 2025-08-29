#include "RTRenderer.hpp"
#include "pch.h"

namespace VRTR
{
    // RTRenderer::~RTRenderer()
    // {
    //     logicalDevice.waitIdle();
    // }

    void RTRenderer::init(GLFWwindow* window)
    {
        VRTR_DEBUG("RTRENDERER INIT");

        initInstance();

        initValidationLayers();

        initPhysicalDeviceAndSurface(window);

        initLogicalDevice();

        initSwapChain();

        initPipeline();
        // this->window = window;
        // VRTR_Instance = std::make_unique<VulkanInstance>(context);
        // VRTR_valLayers = std::make_unique<ValidationLayers>(context);
        // VRTR_PhysicalDevice = std::make_unique<PhysicalDevice>();
        // VRTR_LogicalDevice = std::make_unique<LogicalDevice>();
        // VRTR_WindowSurface = std::make_unique<WindowSurface>();
        // VRTR_RasterGraphicsPipeline = std::make_unique<RasterGraphicsPipeline>();
        // VRTR_CommandBuffer = std::make_unique<CommandBuffer>(commandPool, commandBuffers);


        // VRTR_Instance->createInstance(instance);
        // VRTR_valLayers->setupDebugMessenger(instance);

        // VRTR_WindowSurface->setupSurface(instance, window, surface);

        // VRTR_PhysicalDevice->pickPhysicalDevice(instance, physicalDevice);

        // VRTR_SwapChain = std::make_unique<SwapChain>(logicalDevice,
        //                                             surface, swapChain,
        //                                             swapChainImages, swapChainImageViews
        //                                             );


        // QueueFamilyIndex = PhysicalDevice::findQueueFamilies(physicalDevice, surface);

        // VRTR_LogicalDevice->createLogicalDevice(physicalDevice, logicalDevice, Queue, QueueFamilyIndex);
        // VRTR_SwapChain->createSwapChain(physicalDevice, window);
        // VRTR_SwapChain->createImageViews();
        // surfaceCapabilities = VRTR_SwapChain->getSurfaceCapabilities();

        // VRTR_RasterGraphicsPipeline->createPipeline(logicalDevice, surfaceCapabilities,
        //                                             pipelineLayout, rasterGraphicsPipeline);
        // VRTR_CommandBuffer->createCommandPool(logicalDevice, QueueFamilyIndex);
        // VRTR_CommandBuffer->createCommandBuffers(logicalDevice);
        // createSyncObjects();
    }

    std::vector<const char*> RTRenderer::getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        #ifndef NDEBUG
            if(enableValidationLayers)
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        #endif

        return extensions;
    }

    bool RTRenderer::checkExtensionsSupport(const std::vector<const char*>& glfwExtensions, 
                                            const std::vector<vk::ExtensionProperties>& extensionsProperties)
    {
        std::ranges::for_each(glfwExtensions, 
            [&extensionsProperties](auto const& glfwExtension)
            {
                if (std::ranges::none_of(extensionsProperties,
                    [glfwExtension](auto const& extensionProperty)
                    {
                        return (strcmp(glfwExtension, extensionProperty.extensionName) == 0);
                    }))
                    {
                        throw std::runtime_error("Required extension not supported: " + std::string(glfwExtension));
                    }
            });
        return true;
    }


    void RTRenderer::initInstance()
    {
        VRTR_DEBUG("CREATING VULKAN INSTANCE");
        if(glfwVulkanSupported() != GLFW_TRUE)
        {
            VRTR_CRITICAL("GLFW VULKAN NOT SUPPORTED");
            throw std::runtime_error("GLFW VULKAN NOT SUPPORTED");
        }

        std::vector<vk::ExtensionProperties> availableExtensionProperties = ctx.context.enumerateInstanceExtensionProperties();

        auto extensions = getRequiredExtensions();
        uint32_t extensionsCount = static_cast<uint32_t>(extensions.size());

        vk::ApplicationInfo appInfo{
            .pApplicationName = "Vulkan Ray tracer with radiosity",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = vk::ApiVersion14
        };

        #ifndef NDEBUG
            bool has_debug_utils = std::ranges::any_of(
                availableExtensionProperties,
                [](auto const& ep){ return strcmp(ep.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0;}
            );

            if(enableValidationLayers && !has_debug_utils)
            {
                VRTR_CRITICAL("Validation layer not available!");
                throw std::runtime_error("Validation layer not available!");
            }
        #endif

        if(!checkExtensionsSupport(extensions, availableExtensionProperties))
            VRTR_CRITICAL("GLFW EXTENSION DOES NOT MACH INSTANCE EXTENSIONS");

        #ifndef NDEBUG
            auto debugCI = populateDebugMessengerCreateInfo();
            vk::InstanceCreateInfo createInfo{};
            createInfo.pApplicationInfo = &appInfo;
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
            createInfo.enabledExtensionCount = extensionsCount;
            createInfo.ppEnabledExtensionNames = extensions.data();
            createInfo.pNext = &debugCI;
        #endif

        try
        {
            ctx.instance = vk::raii::Instance(ctx.context, createInfo);
        }
        catch (const vk::SystemError& e)
        {
            VRTR_CRITICAL("Failed to create Vulkan instance: {}", e.what());
            throw;
        }
        catch (const std::exception& e)
        {
            VRTR_CRITICAL("Failed to create Vulkan instance: {}", e.what());
            throw;
        }
    }

    #ifndef NDEBUG

        vk::DebugUtilsMessengerCreateInfoEXT RTRenderer::populateDebugMessengerCreateInfo()
        {
            vk::DebugUtilsMessageSeverityFlagsEXT severityFlags( vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError );
            vk::DebugUtilsMessageTypeFlagsEXT    messageTypeFlags( vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation );
            return vk::DebugUtilsMessengerCreateInfoEXT{
                .messageSeverity = severityFlags,
                .messageType = messageTypeFlags,
                .pfnUserCallback = &debugCallback
            };
        }

        void RTRenderer::initValidationLayers()
        {
            if(!enableValidationLayers) return;

            auto layerProperties = ctx.context.enumerateInstanceLayerProperties();

            bool validationLayersSupported = std::ranges::all_of(
                validationLayers,
                [layerProperties](const char* layerName)
                {
                    return std::ranges::any_of(
                        layerProperties,
                        [layerName](auto const& layerProperty)
                        {
                            return (strcmp(layerName, layerProperty.layerName) == 0);
                        }
                    );
                }
            );

            if (!validationLayersSupported)
            {
                VRTR_CRITICAL("Validation layers requested, but not available!");
                throw std::runtime_error("Validation layers requested, but not available!");
            }

            auto debugCI = populateDebugMessengerCreateInfo();

            ctx.debugMessenger = ctx.instance.createDebugUtilsMessengerEXT(debugCI, nullptr);
        }
    #endif

    bool RTRenderer::isDeviceSuitable(const vk::raii::PhysicalDevice& device)
    {
        bool isSuitable = false;
        vk::PhysicalDeviceProperties properties = device.getProperties();
        std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();
        std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();

        // Physical device needs to support Vulkan 1.4 or higher
        isSuitable = properties.apiVersion >= VK_API_VERSION_1_4;

        const auto& qfpIt = std::ranges::find_if(queueFamilies,
            [](const vk::QueueFamilyProperties& qfp)
            {
                return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlagBits>(0);
            });
        
        // Check if the device has at least one queue family that supports graphics operations
        isSuitable = isSuitable && (qfpIt != queueFamilies.end());

        bool foundExtensions = true;
        for (auto const& extension: deviceExtensions)
        {
            auto extensionIter = std::ranges::find_if(availableExtensions,
                [extension](const vk::ExtensionProperties& ep)
                {
                    return std::strcmp(ep.extensionName, extension) == 0;
                });

            foundExtensions = foundExtensions && (extensionIter != availableExtensions.end());
        }

        // Check if the device supports the required extensions
        isSuitable = isSuitable && foundExtensions;

        return isSuitable;
    }

    uint32_t RTRenderer::findQueueFamilies()
    {
        std::vector<vk::QueueFamilyProperties> queueFamilies = ctx.gpu.getQueueFamilyProperties();
        uint32_t index = 0;

        for (const auto& queueFamily : queueFamilies)
        {
            // As of now, I will just look for a queue family that supports both graphics and presentation
            // I will need to update it later
            if ((queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) && 
                (ctx.gpu.getSurfaceSupportKHR(static_cast<uint32_t>(index), *ctx.surface)))
            {
                VRTR_DEBUG("Found graphics queue family that supports both graphics and presentation at index: {}", index);
                return index;
            }
            index++;
        }
        VRTR_CRITICAL("No suitable graphics queue family found!");
        throw std::runtime_error("No suitable graphics queue family found!");
    }

    void RTRenderer::initPhysicalDeviceAndSurface(GLFWwindow* window)
    {
        VRTR_DEBUG("Selecting Physical Device");
        std::vector<vk::raii::PhysicalDevice> gpus = ctx.instance.enumeratePhysicalDevices();

        for (const auto& gpu : gpus)
        {
            if (isDeviceSuitable(gpu))
            {
                ctx.gpu = gpu;
                VRTR_DEBUG("Physical device selected: {}", ctx.gpu.getProperties().deviceName.data());
            }
        }

        VkSurfaceKHR tempSurface;
        if(glfwCreateWindowSurface(*ctx.instance, window, nullptr, &tempSurface) != VK_SUCCESS)
        {
            VRTR_ERROR("Failed to create window surface");
            throw std::runtime_error("Failed to create window surface");
        }
        ctx.surface = vk::raii::SurfaceKHR(ctx.instance, tempSurface);
        ctx.graphics_queue_index = findQueueFamilies();
    }

    void RTRenderer::initLogicalDevice()
    {

    }

    void RTRenderer::initSwapChain()
    {

    }

    void RTRenderer::initPipeline()
    {

    }

    // void RTRenderer::createSyncObjects()
    // {
    //     VRTR_DEBUG("Creating Sync Objects");

    //     presentCompleteSemaphores.clear();
    //     renderCompleteSemaphores.clear();
    //     drawFences.clear();

    //     vk::SemaphoreCreateInfo semaphoreInfo
    //     {
    //         .pNext = nullptr,
    //         .flags = {}
    //     };

    //     vk::FenceCreateInfo fenceInfo
    //     {
    //         .pNext = nullptr,
    //         .flags = vk::FenceCreateFlagBits::eSignaled // so we don't wait forever the first time
    //     };
    //     for(uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    //         presentCompleteSemaphores.emplace_back(vk::raii::Semaphore(logicalDevice, semaphoreInfo));
    //         renderCompleteSemaphores.emplace_back(vk::raii::Semaphore(logicalDevice, semaphoreInfo));
    //         drawFences.emplace_back(vk::raii::Fence(logicalDevice, fenceInfo));
    //     }
    // }

    // void RTRenderer::recordCommandBuffer(uint32_t imageIndex)
    // {
    //     // TODO think about updating only necessary things inside surfaceCapabilities
    //     // I think it will be only window size and extent???
    //     surfaceCapabilities = VRTR_SwapChain->getSurfaceCapabilities();
    //     commandBuffers.at(currentFrame).begin({});
    //     VRTR_CommandBuffer->transition_image_layout
    //     (
    //         swapChainImages, imageIndex,
    //         vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
    //         vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eColorAttachmentWrite,
    //         vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eColorAttachmentOutput
    //     ); 

    //     vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    //     vk::RenderingAttachmentInfo attachmentInfo = 
    //     {
    //         .imageView = swapChainImageViews.at(currentFrame),
    //         .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
    //         .resolveMode = vk::ResolveModeFlagBits::eNone,
    //         .loadOp = vk::AttachmentLoadOp::eClear,
    //         .storeOp = vk::AttachmentStoreOp::eStore,
    //         .clearValue = clearColor
    //     };

    //     vk::RenderingInfo renderingInfo = 
    //     {
    //         .flags = {},
    //         .renderArea = {.offset = {0, 0}, .extent = surfaceCapabilities.extent},
    //         .layerCount = 1,
    //         .viewMask = 0,
    //         .colorAttachmentCount = 1,
    //         .pColorAttachments = &attachmentInfo,
    //         .pDepthAttachment = nullptr,
    //         .pStencilAttachment = nullptr
    //     };

    //     commandBuffers.at(currentFrame).beginRendering(renderingInfo);
    //     commandBuffers.at(currentFrame).bindPipeline(vk::PipelineBindPoint::eGraphics, rasterGraphicsPipeline);

    //     // Setting dynamic states
    //     commandBuffers.at(currentFrame).setViewport(0, vk::Viewport{0.0f, 0.0f,
    //         static_cast<float>(surfaceCapabilities.extent.width),
    //         static_cast<float>(surfaceCapabilities.extent.height), 0.0f, 1.0f});
    //     commandBuffers.at(currentFrame).setScissor(0, vk::Rect2D{{0, 0}, surfaceCapabilities.extent});
    //     commandBuffers.at(currentFrame).draw(3, 1, 0, 0);

    //     commandBuffers.at(currentFrame).endRendering();

    //     VRTR_CommandBuffer->transition_image_layout(
    //         swapChainImages, imageIndex,
    //         vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
    //         vk::AccessFlagBits2::eColorAttachmentWrite, vk::AccessFlagBits2::eNone,
    //         vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eNone
    //     );

    //     commandBuffers.at(currentFrame).end();
    // }

    // void RTRenderer::drawFrame()
    // {
    //     while (vk::Result::eTimeout == logicalDevice.waitForFences(*drawFences.at(currentFrame), VK_TRUE, UINT64_MAX))
    //     ;

    //     // Unfortunately it needs to be inside try catch block, because "acquireNextImage" is throwing exceptions
    //     // I cant disable it, because i am using vk::raii and it requires exceptions to be enabled :(
    //     try
    //     {
    //         auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, presentCompleteSemaphores.at(semaphoreIndex), nullptr);

    //     recordCommandBuffer(imageIndex);
    //     logicalDevice.resetFences({drawFences[currentFrame]});

    //     vk::PipelineStageFlags waitDestinationStageMask( vk::PipelineStageFlagBits::eColorAttachmentOutput );
    //     const vk::SubmitInfo submitInfo
    //     {
    //         .pNext = nullptr,
    //         .waitSemaphoreCount = 1,
    //         .pWaitSemaphores = &*presentCompleteSemaphores.at(semaphoreIndex),
    //         .pWaitDstStageMask = &waitDestinationStageMask,
    //         .commandBufferCount = 1,
    //         .pCommandBuffers = &*commandBuffers.at(currentFrame),
    //         .signalSemaphoreCount = 1,
    //         .pSignalSemaphores = &*renderCompleteSemaphores.at(currentFrame)
    //     };
    //     Queue.submit({submitInfo}, *drawFences.at(currentFrame));
    //     const vk::PresentInfoKHR presentInfoKHR{
    //         .pNext = nullptr,
    //         .waitSemaphoreCount = 1,
    //         .pWaitSemaphores = &*renderCompleteSemaphores.at(currentFrame),
    //         .swapchainCount = 1,
    //         .pSwapchains = &*swapChain,
    //         .pImageIndices = &imageIndex,
    //         .pResults = nullptr
    //     };

    //     result = Queue.presentKHR(presentInfoKHR);

    //     VRTR::semaphoreIndex = (VRTR::semaphoreIndex + 1) % presentCompleteSemaphores.size();
    //     VRTR::currentFrame = (VRTR::currentFrame + 1) % VRTR::MAX_FRAMES_IN_FLIGHT;

    //     }
    //     catch (const vk::OutOfDateKHRError& e)
    //     {
    //         VRTR_SwapChain->recreateSwapChain(physicalDevice, window);
    //         return;
    //     }
    //     catch (const std::exception& e)
    //     {
    //         VRTR_CRITICAL("Failed to acquire swap chain image!");
    //         throw std::runtime_error("Failed to acquire swap chain image!");
    //     }
    // }

    VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                   vk::DebugUtilsMessageTypeFlagsEXT type,
                                                   const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                   void*)
    {
        switch (severity) {
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
                VRTR_VALIDATION_TRACE("Message: {}", pCallbackData->pMessage);
                break;
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
                VRTR_VALIDATION_INFO("Message: {}", pCallbackData->pMessage);
                break;
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
                VRTR_VALIDATION_WARN("Message: {}", pCallbackData->pMessage);
                break;
            case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
                VRTR_VALIDATION_ERROR("Message: {}", pCallbackData->pMessage);
                break;
            default:
                break;
        }

        return VK_FALSE;
    }
}
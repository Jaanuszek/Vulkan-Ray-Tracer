#include "RTRenderer.hpp"
#include "pch.h"

namespace VRTR
{
    RTRenderer::~RTRenderer()
    {
        ctx.logicalDevice.waitIdle();
    }

    void RTRenderer::init(GLFWwindow* window)
    {
        VRTR_DEBUG("RTRENDERER INIT");

        initInstance();

        initValidationLayers();

        initPhysicalDeviceAndSurface(window);

        initRayTracing();

        initLogicalDevice();

        initSwapChain(window);

        ctx.vertex_buffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, sizeof(vertices[0]) * vertices.size(), vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        ctx.vertex_buffer->Update(vertices.data(), sizeof(vertices[0]) * vertices.size());

        initPipeline();

        initCommandBuffer();

        createSyncObjects();

        createBLAS();
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

        #if defined(_HPP_VULKAN_LIBRARY)
            static vk::detail::DynamicLoader dl(_HPP_VULKAN_LIBRARY);
        #else
            static vk::detail::DynamicLoader dl;
        #endif
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        VRTR_DEBUG("CREATING VULKAN INSTANCE");
        if(glfwVulkanSupported() != GLFW_TRUE)
        {
            VRTR_CRITICAL("GLFW VULKAN NOT SUPPORTED");
            throw std::runtime_error("GLFW VULKAN NOT SUPPORTED");
        }

        std::vector<vk::ExtensionProperties> availableExtensionProperties = ctx.context.enumerateInstanceExtensionProperties();

        auto extensions = getRequiredExtensions();
        uint32_t extensionsCount = static_cast<uint32_t>(extensions.size());

        uint32_t instanceVersion = vk::enumerateInstanceVersion();

        vk::ApplicationInfo appInfo{
            .pApplicationName = "Vulkan Ray tracer with radiosity",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_MAKE_VERSION(
                VK_VERSION_MAJOR(instanceVersion),
                VK_VERSION_MINOR(instanceVersion),
                VK_VERSION_PATCH(instanceVersion))
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
                break;
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
        VRTR_DEBUG("CREATING LOGICAL DEVICE");

        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = ctx.gpu.getQueueFamilyProperties();
        float queuePriority = 0.0f;

        vk::StructureChain<vk::PhysicalDeviceFeatures2,
                            vk::PhysicalDeviceVulkan11Features, 
                            vk::PhysicalDeviceVulkan13Features,
                            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
                            vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
                            vk::PhysicalDeviceBufferDeviceAddressFeatures
                            > featuresChain
        {
            {},
            {.shaderDrawParameters = VK_TRUE},
            {
                .synchronization2 = VK_TRUE,
                .dynamicRendering = VK_TRUE
            },
            {.extendedDynamicState = VK_TRUE},
            {.rayTracingPipeline = VK_TRUE},
            {.bufferDeviceAddress = VK_TRUE}
        };

        vk::DeviceQueueCreateInfo queueCreateInfo
        {
            .queueFamilyIndex = static_cast<uint32_t>(ctx.graphics_queue_index),
            .queueCount = 1,
            .pQueuePriorities = &queuePriority 
        };
        
        // enabledLayerCount and ppEnabledLayersNames are not used in Vulkan 1.4
        vk::DeviceCreateInfo deviceCreateInfo
        {
            .pNext = &featuresChain.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
            .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
            .ppEnabledExtensionNames = deviceExtensions.data()
        };

        ctx.logicalDevice = vk::raii::Device(ctx.gpu, deviceCreateInfo);
        ctx.queue = vk::raii::Queue(ctx.logicalDevice, static_cast<uint32_t>(ctx.graphics_queue_index), 0);
    }

    void RTRenderer::initSwapChain(GLFWwindow* window)
    {
        VRTR_SwapChain = std::make_unique<SwapChainManager>(ctx);
        VRTR_SwapChain->createSwapChain(window);
        VRTR_SwapChain->createImageViews();
        surfaceCapabilities = VRTR_SwapChain->getSurfaceCapabilities();
    }

    uint32_t RTRenderer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
    {
        for (uint32_t i = 0; i < ctx.gpu.getMemoryProperties().memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (ctx.gpu.getMemoryProperties().memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable memory type");
    }

    void RTRenderer::initPipeline()
    {
        VRTR_DEBUG("CREATING PIPELINE");
        VRTR_RasterGraphicsPipeline = std::make_unique<RasterGraphicsPipeline>(ctx);
        VRTR_RasterGraphicsPipeline->createPipeline(surfaceCapabilities.surfaceFormat.format);
    }

    void RTRenderer::initCommandBuffer()
    {
        VRTR_CommandBuffer = std::make_unique<CommandBuffer>(ctx);
        VRTR_CommandBuffer->createCommandPool();
        VRTR_CommandBuffer->createCommandBuffers();
    }

    void RTRenderer::createSyncObjects()
    {
        VRTR_DEBUG("Creating Sync Objects");

        ctx.presentCompleteSemaphores.clear();
        ctx.renderCompleteSemaphores.clear();
        ctx.drawFences.clear();

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
            ctx.presentCompleteSemaphores.emplace_back(vk::raii::Semaphore(ctx.logicalDevice, semaphoreInfo));
            ctx.renderCompleteSemaphores.emplace_back(vk::raii::Semaphore(ctx.logicalDevice, semaphoreInfo));
            ctx.drawFences.emplace_back(vk::raii::Fence(ctx.logicalDevice, fenceInfo));
        }
    }

    void RTRenderer::recordCommandBuffer(uint32_t imageIndex)
    {
        // TODO think about updating only necessary things inside surfaceCapabilities
        // I think it will be only window size and extent???
        surfaceCapabilities = VRTR_SwapChain->getSurfaceCapabilities();
        ctx.commandBuffers.at(currentFrame).begin({});
        VRTR_CommandBuffer->transition_image_layout
        (
            ctx.swapChainImages, imageIndex,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
            vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eColorAttachmentOutput
        ); 

        vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
        vk::RenderingAttachmentInfo attachmentInfo = 
        {
            .imageView = ctx.swapChainImageViews.at(currentFrame),
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

        ctx.commandBuffers.at(currentFrame).beginRendering(renderingInfo);
        ctx.commandBuffers.at(currentFrame).bindPipeline(vk::PipelineBindPoint::eGraphics, ctx.pipeline);
        ctx.commandBuffers.at(currentFrame).bindVertexBuffers(0, {ctx.vertex_buffer->getBuffer()}, {0});

        // Setting dynamic states
        ctx.commandBuffers.at(currentFrame).setViewport(
            0, 
            vk::Viewport
            {
                0.0f, 
                0.0f,
                static_cast<float>(surfaceCapabilities.extent.width),
                static_cast<float>(surfaceCapabilities.extent.height), 
                0.0f, 
                1.0f
            }    
        );
        ctx.commandBuffers.at(currentFrame).setScissor(0, vk::Rect2D{{0, 0}, surfaceCapabilities.extent});
        ctx.commandBuffers.at(currentFrame).draw(vertices.size(), 1, 0, 0);

        ctx.commandBuffers.at(currentFrame).endRendering();

        VRTR_CommandBuffer->transition_image_layout(
            ctx.swapChainImages, imageIndex,
            vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite, vk::AccessFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eNone
        );

        ctx.commandBuffers.at(currentFrame).end();
    }

    void RTRenderer::initRayTracing()
    {
        vk::PhysicalDeviceProperties2 properties2
        {
            .pNext = &rayTracingPipelineProperties
        };
        auto prop = ctx.gpu.getProperties2<vk::PhysicalDeviceProperties2,
            vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

        auto rtProps = prop.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

        VRTR_DEBUG("RT vars {}", rtProps.maxRayRecursionDepth);
    }

    void RTRenderer::createBLAS()
    {
        struct VertexRT
        {
            glm::vec3 pos;
        };

        std::vector<VertexRT> verticesRT = {
            {{1.0f, 1.0f, 0.0f}},
            {{-1.0f, 1.0f, 0.0f}},
            {{0.0f, -1.0f, 0.0f}}
        };
        std::vector<uint32_t> indicesRT = {0, 1, 2};

        size_t vertex_buffer_size = verticesRT.size() * sizeof(VertexRT);
        size_t index_buffer_size = indicesRT.size() * sizeof(uint32_t);

        const vk::BufferUsageFlags buffer_usage_flags = vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        const vk::MemoryPropertyFlags memory_property_flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

        vertex_buffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, vertex_buffer_size, buffer_usage_flags, memory_property_flags);
        vertex_buffer->Update(verticesRT.data(), vertex_buffer_size);

        index_buffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, index_buffer_size, buffer_usage_flags, memory_property_flags);
        index_buffer->Update(indicesRT.data(), index_buffer_size);

        vk::TransformMatrixKHR transformMatrix{
            std::array<std::array<float, 4>, 3>{
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f
            }
        };
        std::unique_ptr<Buffer> transform_matrix_buffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, sizeof(vk::TransformMatrixKHR), vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        transform_matrix_buffer->Update(&transformMatrix, sizeof(vk::TransformMatrixKHR));

        vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
        vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
        vk::DeviceOrHostAddressConstKHR transformMatrixDeviceAddress{};

        vertexBufferDeviceAddress.deviceAddress = vertex_buffer->getDeviceAddress();
        indexBufferDeviceAddress.deviceAddress = index_buffer->getDeviceAddress();
        transformMatrixDeviceAddress.deviceAddress = transform_matrix_buffer->getDeviceAddress();

    }

    void RTRenderer::createTLAS()
    {

    }

    void RTRenderer::drawFrame(GLFWwindow* window)
    {
        while (vk::Result::eTimeout == ctx.logicalDevice.waitForFences(*ctx.drawFences.at(currentFrame), VK_TRUE, UINT64_MAX))
        ;

        // Unfortunately it needs to be inside try catch block, because "acquireNextImage" is throwing exceptions
        // I cant disable it, because i am using vk::raii and it requires exceptions to be enabled :(
        try
        {
            auto [result, imageIndex] = ctx.swapChain.acquireNextImage(UINT64_MAX, ctx.presentCompleteSemaphores.at(semaphoreIndex), nullptr);

        recordCommandBuffer(imageIndex);
        ctx.logicalDevice.resetFences({ctx.drawFences[currentFrame]});

        vk::PipelineStageFlags waitDestinationStageMask( vk::PipelineStageFlagBits::eColorAttachmentOutput );
        const vk::SubmitInfo submitInfo
        {
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*ctx.presentCompleteSemaphores.at(semaphoreIndex),
            .pWaitDstStageMask = &waitDestinationStageMask,
            .commandBufferCount = 1,
            .pCommandBuffers = &*ctx.commandBuffers.at(currentFrame),
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &*ctx.renderCompleteSemaphores.at(currentFrame)
        };
        ctx.queue.submit({submitInfo}, *ctx.drawFences.at(currentFrame));
        const vk::PresentInfoKHR presentInfoKHR{
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*ctx.renderCompleteSemaphores.at(currentFrame),
            .swapchainCount = 1,
            .pSwapchains = &*ctx.swapChain,
            .pImageIndices = &imageIndex,
            .pResults = nullptr
        };

        result = ctx.queue.presentKHR(presentInfoKHR);

        VRTR::semaphoreIndex = (VRTR::semaphoreIndex + 1) % ctx.presentCompleteSemaphores.size();
        VRTR::currentFrame = (VRTR::currentFrame + 1) % VRTR::MAX_FRAMES_IN_FLIGHT;

        }
        catch (const vk::OutOfDateKHRError& e)
        {
            VRTR_SwapChain->recreateSwapChain(window);
            return;
        }
        catch (const std::exception& e)
        {
            VRTR_CRITICAL("Failed to acquire swap chain image!");
            throw std::runtime_error("Failed to acquire swap chain image!");
        }
    }

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
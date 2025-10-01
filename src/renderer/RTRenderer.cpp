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

        glfwGetFramebufferSize(window, &width, &height);

        //  TODO Przeniesc to do jakies funkcji ktora ustawia wszystko
        camera = std::make_unique<Camera>();
        camera->setPerspective(45.0f, static_cast<float>(width) / height, 0.1f, 100.0f);
        camera->setTranslation(glm::vec3(0.0f, 0.0f, -3.0f));
        camera->setRotation(glm::vec3(0.0f, 0.0f, 0.0f));

        initInstance();

        initValidationLayers();

        initPhysicalDeviceAndSurface(window);

        initRayTracing();

        initLogicalDevice();

        // TODO Przeniesc to do jakies funkcji ktora ustawia wszystko
        uniform_buffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, sizeof(UniformData), 
                                            vk::BufferUsageFlagBits::eUniformBuffer, 
                                            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        updateUniformBuffer();

        initSwapChain(window);

        ctx.vertex_buffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, sizeof(vertices[0]) * vertices.size(), vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        ctx.vertex_buffer->Update(vertices.data(), sizeof(vertices[0]) * vertices.size());

        initCommandBuffer();

        createSyncObjects();

        createStorageImage();

        createScene();

        createRayTracingPipeline();

        createShaderBindingTable();

        createDescriptorSets();

        buildRTCommandBuffers();
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
                            vk::PhysicalDeviceRayQueryFeaturesKHR,
                            vk::PhysicalDeviceBufferDeviceAddressFeatures,
                            vk::PhysicalDeviceAccelerationStructureFeaturesKHR
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
            {.rayQuery = VK_TRUE},
            {.bufferDeviceAddress = VK_TRUE},
            {.accelerationStructure = VK_TRUE}
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

    void RTRenderer::updateUniformBuffer()
    {
        uniform_data.proj_inverse = glm::inverse(camera->matrices.perspective);
        uniform_data.view_inverse = glm::inverse(camera->matrices.view);
        uniform_buffer->Update(&uniform_data, sizeof(UniformData));
    }
    
    void RTRenderer::initRayTracing()
    {
        auto prop = ctx.gpu.getProperties2<vk::PhysicalDeviceProperties2,
            vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

        rayTracingPipelineProperties = prop.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
    }

    ScratchBuffer RTRenderer::createScratchBuffer(vk::DeviceSize size)
    {
        ScratchBuffer scratchBuffer{};
        
        vk::BufferCreateInfo bufferCreateInfo
        {
            .size = size,
            .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress
        };
        scratchBuffer.buffer = vk::raii::Buffer(ctx.logicalDevice, bufferCreateInfo);

        vk::MemoryRequirements memRequirements = scratchBuffer.buffer.getMemoryRequirements();

        vk::MemoryAllocateFlagsInfo allocateFlagsInfo
        {
            .pNext = nullptr,
            .flags = vk::MemoryAllocateFlagBits::eDeviceAddress,
        };

        uint32_t memoryType = Buffer::findMemoryType(ctx.gpu, memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

        vk::MemoryAllocateInfo allocInfo
        {
            .pNext = &allocateFlagsInfo,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = memoryType
        };
        scratchBuffer.memory = vk::raii::DeviceMemory(ctx.logicalDevice, allocInfo);
        scratchBuffer.buffer.bindMemory(*scratchBuffer.memory, 0);

        vk::BufferDeviceAddressInfo bufferDeviceAddressInfo
        {
            .buffer = scratchBuffer.buffer
        };

        scratchBuffer.device_address = ctx.logicalDevice.getBufferAddress(bufferDeviceAddressInfo);

        return scratchBuffer;
    }

    void RTRenderer::createBLAS()
    {
        VRTR_DEBUG("Creating BLAS");
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

        vk::AccelerationStructureGeometryTrianglesDataKHR triangles
        {
            .pNext = nullptr,
            .vertexFormat = vk::Format::eR32G32B32Sfloat,
            .vertexData = vertexBufferDeviceAddress,
            .vertexStride = sizeof(VertexRT),
            .maxVertex = static_cast<uint32_t>(verticesRT.size()),
            .indexType = vk::IndexType::eUint32,
            .indexData = indexBufferDeviceAddress,
            .transformData = transformMatrixDeviceAddress
        };

        vk::AccelerationStructureGeometryKHR asGeometry
        {
            .pNext = nullptr,
            .geometryType = vk::GeometryTypeKHR::eTriangles,
            .geometry = triangles,
            .flags = vk::GeometryFlagBitsKHR::eOpaque
        };

        vk::AccelerationStructureBuildRangeInfoKHR offsetInfo
        {
            .primitiveCount = 1,
            .primitiveOffset = 0,
            .firstVertex = 0,
            .transformOffset = 0
        };

        // inicjacja struktury acceleration structure.

        vk::AccelerationStructureBuildGeometryInfoKHR buildInfoStructure
        {
            .pNext = nullptr,
            .type = vk::AccelerationStructureTypeKHR::eBottomLevel,
            .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            .geometryCount = 1,
            .pGeometries = &asGeometry,
            .ppGeometries = nullptr,
        };

        vk::AccelerationStructureBuildSizesInfoKHR sizeInfo =
            ctx.logicalDevice.getAccelerationStructureBuildSizesKHR(
                vk::AccelerationStructureBuildTypeKHR::eDevice,
                buildInfoStructure,
                {offsetInfo.primitiveCount}
            ); 
        
        blas_structure.buffer = std::make_unique<Buffer>(
            ctx.logicalDevice, ctx.gpu, sizeInfo.accelerationStructureSize,
            vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );

        vk::AccelerationStructureCreateInfoKHR asCreateInfo = 
        {
            .pNext = nullptr,
            .createFlags = {},
            .buffer = blas_structure.buffer->getBuffer(),
            .offset = 0,
            .size = sizeInfo.accelerationStructureSize,
            .type = vk::AccelerationStructureTypeKHR::eBottomLevel,
            .deviceAddress = 0
        };
        blas_structure.handle = vk::raii::AccelerationStructureKHR(ctx.logicalDevice, asCreateInfo);

        ScratchBuffer scratchBuffer = createScratchBuffer(sizeInfo.buildScratchSize);

        // Inicjacja juz docelowej struktury BLAS

        vk::AccelerationStructureBuildGeometryInfoKHR buildInfo
        {
            .pNext = nullptr,
            .type = vk::AccelerationStructureTypeKHR::eBottomLevel,
            .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            .mode = vk::BuildAccelerationStructureModeKHR::eBuild,
            .srcAccelerationStructure = nullptr,
            .dstAccelerationStructure = *blas_structure.handle,
            .geometryCount = 1,
            .pGeometries = &asGeometry,
            .ppGeometries = nullptr,
            .scratchData = scratchBuffer.device_address
        };

        std::array<vk::AccelerationStructureBuildRangeInfoKHR*, 1> buildRangeInfos = {&offsetInfo};

        // temp commandbuffer
        // JAK COS BEDZIE NIE TAK TO TUTAJ SPRAWDZIC!!!
        vk::CommandBufferAllocateInfo allocInfo
        {
            .commandPool = ctx.commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };

        vk::raii::CommandBuffer tmpCommandBuffer = std::move(ctx.logicalDevice.allocateCommandBuffers(allocInfo).front());
        tmpCommandBuffer.begin({});
        tmpCommandBuffer.buildAccelerationStructuresKHR(
            {buildInfo},
            buildRangeInfos
        );
        tmpCommandBuffer.end();
        vk::SubmitInfo submitInfo
        {
            .commandBufferCount = 1,
            .pCommandBuffers = &*tmpCommandBuffer,
        };

        vk::raii::Fence fence = ctx.logicalDevice.createFence({});
        ctx.queue.submit({submitInfo}, fence);
        auto result = ctx.logicalDevice.waitForFences(*fence, VK_TRUE, UINT64_MAX);
        if(result != vk::Result::eSuccess)
        {
            VRTR_CRITICAL("Failed to wait for fence after BLAS build!");
        }

        vk::AccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo
        {
            .pNext = nullptr,
            .accelerationStructure = *blas_structure.handle
        };
        blas_structure.device_address = ctx.logicalDevice.getAccelerationStructureAddressKHR(accelerationStructureDeviceAddressInfo);
    }

    void RTRenderer::createTLAS()
    {
        VRTR_DEBUG("Creating TLAS");
        vk::TransformMatrixKHR transformMatrix{
            std::array<std::array<float, 4>, 3>{
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f
            }
        };

        vk::AccelerationStructureInstanceKHR ac_instance
        {
            .transform = transformMatrix,
            .instanceCustomIndex = 0,
            .mask = 0xFF,
            .instanceShaderBindingTableRecordOffset = 0,
            .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
            .accelerationStructureReference = blas_structure.device_address
        };

        std::unique_ptr<Buffer> instance_buffer = std::make_unique<Buffer>(ctx.logicalDevice, 
                                ctx.gpu, 
                                sizeof(vk::AccelerationStructureInstanceKHR), 
                                vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress, 
                                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        
        instance_buffer->Update(&ac_instance, sizeof(vk::AccelerationStructureInstanceKHR));

        vk::DeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
        instanceDataDeviceAddress.deviceAddress = instance_buffer->getDeviceAddress();

        vk::AccelerationStructureGeometryKHR ASGeometry
        {
            .pNext = nullptr,
            .geometryType = vk::GeometryTypeKHR::eInstances,
            .geometry = vk::AccelerationStructureGeometryInstancesDataKHR{
                .pNext = nullptr,
                .arrayOfPointers = VK_FALSE,
                .data = instanceDataDeviceAddress
            },
            .flags = vk::GeometryFlagBitsKHR::eOpaque
        };

        vk::AccelerationStructureBuildGeometryInfoKHR ASBuildGeometryInfo
        {
            .pNext = nullptr,
            .type = vk::AccelerationStructureTypeKHR::eTopLevel,
            .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            .geometryCount = 1,
            .pGeometries = &ASGeometry,
            .ppGeometries = nullptr
        };

        const uint32_t primitive_count = 1;

        vk::AccelerationStructureBuildSizesInfoKHR ASBuildSizeInfo =
            ctx.logicalDevice.getAccelerationStructureBuildSizesKHR(
                vk::AccelerationStructureBuildTypeKHR::eDevice,
                ASBuildGeometryInfo,
                {primitive_count}
            );

        tlas_structure.buffer = std::make_unique<Buffer>(
            ctx.logicalDevice, ctx.gpu,
            ASBuildSizeInfo.accelerationStructureSize,
            vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );

        vk::AccelerationStructureCreateInfoKHR ASCreateInfo
        {
            .pNext = nullptr,
            .createFlags = {},
            .buffer = tlas_structure.buffer->getBuffer(),
            .offset = 0,
            .size = ASBuildSizeInfo.accelerationStructureSize,
            .type = vk::AccelerationStructureTypeKHR::eTopLevel,
            .deviceAddress = 0
        };

        tlas_structure.handle = vk::raii::AccelerationStructureKHR(ctx.logicalDevice, ASCreateInfo);

        ScratchBuffer scratchBuffer = createScratchBuffer(ASBuildSizeInfo.buildScratchSize);

        vk::AccelerationStructureBuildGeometryInfoKHR destBuildInfo
        {
            .pNext = nullptr,
            .type = vk::AccelerationStructureTypeKHR::eTopLevel,
            .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
            .mode = vk::BuildAccelerationStructureModeKHR::eBuild,
            .srcAccelerationStructure = nullptr,
            .dstAccelerationStructure = tlas_structure.handle,
            .geometryCount = 1,
            .pGeometries = &ASGeometry,
            .scratchData = scratchBuffer.device_address
        };

        vk::AccelerationStructureBuildRangeInfoKHR offsetInfo
        {
            .primitiveCount = primitive_count,
            .primitiveOffset = 0,
            .firstVertex = 0,
            .transformOffset = 0
        };

        std::array<vk::AccelerationStructureBuildRangeInfoKHR*, 1> buildRangeInfos = {&offsetInfo};

        vk::CommandBufferAllocateInfo allocInfo
        {
            .commandPool = ctx.commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };
        vk::raii::CommandBuffer tmpCommandBuffer = CommandBuffer::createTempCommandBuffer(ctx, vk::CommandBufferLevel::ePrimary, true);

        tmpCommandBuffer.buildAccelerationStructuresKHR(
            {destBuildInfo},
            buildRangeInfos
        );

        CommandBuffer::flushTempCommandBuffer(ctx, tmpCommandBuffer);

        vk::AccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo
        {
            .pNext = nullptr,
            .accelerationStructure = tlas_structure.handle
        };
        tlas_structure.device_address = ctx.logicalDevice.getAccelerationStructureAddressKHR(accelerationStructureDeviceAddressInfo);
    }

    void RTRenderer::createScene()
    {
        VRTR_DEBUG("Creating scene");
        createBLAS();
        createTLAS();
    }

    void RTRenderer::createStorageImage()
    {
        VRTR_DEBUG("Creating storage image");
        storageImage.width = static_cast<uint32_t>(width);
        storageImage.height = static_cast<uint32_t>(height);

        vk::ImageCreateInfo imgCreateInfo
        {
            .imageType = vk::ImageType::e2D,
            .format = vk::Format::eR8G8B8A8Unorm,
            .extent = vk::Extent3D{storageImage.width, storageImage.height, 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eOptimal,
            .usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc,
            .sharingMode = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined
        };
        storageImage.image = vk::raii::Image(ctx.logicalDevice, imgCreateInfo);

        vk::MemoryRequirements memRequirements = storageImage.image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo
        {
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
        };
        storageImage.memory = vk::raii::DeviceMemory(ctx.logicalDevice, allocInfo);
        storageImage.image.bindMemory(*storageImage.memory, 0);

        vk::ImageViewCreateInfo viewCreateInfo
        {
            .image = *storageImage.image,
            .viewType = vk::ImageViewType::e2D,
            .format = vk::Format::eR8G8B8A8Unorm,
            .components = {
                vk::ComponentSwizzle::eIdentity, // it has to be identity inside storageImage
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity
            },
            .subresourceRange = {
                vk::ImageAspectFlagBits::eColor,
                0, 1, 0, 1
            }
        };
        storageImage.imageView = vk::raii::ImageView(ctx.logicalDevice, viewCreateInfo);

        // TODO OGARNAC TE TYMCZASOWE COMMAND BUFFERY
        vk::CommandBufferAllocateInfo cmdBufferAllocInfo
        {
            .commandPool = ctx.commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };

        vk::raii::CommandBuffer tmpCmdBuffer = CommandBuffer::createTempCommandBuffer(ctx, vk::CommandBufferLevel::ePrimary, true);

        VRTR_CommandBuffer->transition_image_layout
        (
            tmpCmdBuffer,
            storageImage.image, 
            vk::ImageLayout::eUndefined, 
            vk::ImageLayout::eGeneral, // it's basicaly storage image flag - we can do everything with it copy/write/read
            vk::AccessFlagBits2::eNone, 
            vk::AccessFlagBits2::eShaderWrite, // ?????????
            vk::PipelineStageFlagBits2::eAllCommands, // CHANGE IT LATER. It's very slow since GPU has to wait for all previous commands to finish 
            vk::PipelineStageFlagBits2::eAllCommands // CHANGE IT LATER
        );

        VRTR::CommandBuffer::flushTempCommandBuffer(ctx, tmpCmdBuffer);
    }

    void RTRenderer::createDescriptorSets()
    {
        VRTR_DEBUG("Creating Descriptor Sets");
        uint32_t maxSets = 1; // one for now, but later we will need more
        std::vector<vk::DescriptorPoolSize> poolSizes=
        {
            {vk::DescriptorType::eAccelerationStructureKHR, maxSets}, // wsparcie dla AS
            {vk::DescriptorType::eStorageImage, maxSets}, // umozliwienie zapisywania wyniku shaderow do storage image
            {vk::DescriptorType::eUniformBuffer, maxSets} // wsparcie dla uniform bufferow (info ze sceny. np. macierz mvp)
        };

        // Descriptor Pool - zarządzanie pamiecią dla descriptor setów
        vk::DescriptorPoolCreateInfo poolInfo
        {
            .flags = {vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet},
            .maxSets = maxSets,
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes = poolSizes.data()
        };
        descriptorPool = vk::raii::DescriptorPool(ctx.logicalDevice, poolInfo);

        // Descriptor set - opis zasobów używanych przez shadery, 
        // czyli layouty, bindingi ktore potem sie wykorzystujew shaderach
        vk::DescriptorSetAllocateInfo allocInfo
        {
            .descriptorPool = descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &*descriptorSetLayout
        };

        // A little workaround here, because Its not possible to create a single descriptor set in hpp vulkan
        // So I create descriptorSets (NOTE S on the end) and then move the first one to descriptorSet
        // https://github.com/KhronosGroup/Vulkan-Hpp/blob/938a2c36d2d3886a293c63c9a26417d6b0e2bc2d/vk_raii_ProgrammingGuide.md#09-create-a-vkraiidescriptorpool-and-vkraiidescriptorsets

        vk::raii::DescriptorSets tempDescriptorSets = vk::raii::DescriptorSets(ctx.logicalDevice, allocInfo);
        descriptorSet = std::move(tempDescriptorSets.front());

        vk::WriteDescriptorSetAccelerationStructureKHR descriptorASInfo
        {
            .pNext = nullptr,
            .accelerationStructureCount = 1,
            .pAccelerationStructures = &*tlas_structure.handle
        };

        vk::WriteDescriptorSet ASWrite
        {
            .pNext = &descriptorASInfo,
            .dstSet = *descriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eAccelerationStructureKHR,
        };

        vk::DescriptorImageInfo imageInfo
        {
            .sampler = {},
            .imageView = *storageImage.imageView,
            .imageLayout = vk::ImageLayout::eGeneral
        };

        vk::DescriptorBufferInfo bufferInfo
        {
            .buffer = uniform_buffer->getBuffer(),
            .offset = 0,
            .range = vk::WholeSize
        };

        vk::WriteDescriptorSet resultImageWrite
        {
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageImage,
            .pImageInfo = &imageInfo
        };

        vk::WriteDescriptorSet uniformBufferWrite
        {
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &bufferInfo
        };

        std::array<vk::WriteDescriptorSet, 3> WriteDescriptorSets = {
            ASWrite,
            resultImageWrite,
            uniformBufferWrite
        };
        ctx.logicalDevice.updateDescriptorSets(WriteDescriptorSets, {});
    }

    void RTRenderer::createRayTracingPipeline()
    {
        VRTR_DEBUG("Creating Ray Tracing Pipeline");
        vk::DescriptorSetLayoutBinding ASLayout
        {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eAccelerationStructureKHR,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR,
            .pImmutableSamplers = nullptr
        };

        vk::DescriptorSetLayoutBinding storageImageLayout
        {
            .binding = 1,
            .descriptorType = vk::DescriptorType::eStorageImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR,
            .pImmutableSamplers = nullptr
        };

        vk::DescriptorSetLayoutBinding uniformBufferLayout
        {
            .binding = 2,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR,
            .pImmutableSamplers = nullptr
        };

        std::array<vk::DescriptorSetLayoutBinding, 3> bindings = 
        {
            ASLayout,
            storageImageLayout,
            uniformBufferLayout
        };

        vk::DescriptorSetLayoutCreateInfo layoutInfo
        {
            .flags = {},
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()
        };
        descriptorSetLayout = vk::raii::DescriptorSetLayout(ctx.logicalDevice, layoutInfo);

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo
        {
            .flags = {},
            .setLayoutCount = 1,
            .pSetLayouts = &*descriptorSetLayout,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr
        };
        rayTracingPipelineLayout = vk::raii::PipelineLayout(ctx.logicalDevice, pipelineLayoutInfo);

        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
        Shader shader;

        // Raygen shader
        // {
            shaderStages.push_back(shader.createShaderStageInfo(ctx.logicalDevice, "shaders/raygen.spv", vk::ShaderStageFlagBits::eRaygenKHR));
            vk::RayTracingShaderGroupCreateInfoKHR raygenGroup
            {
                .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
                .generalShader = 0, // first entry in shaderStages
                .closestHitShader = VK_SHADER_UNUSED_KHR,
                .anyHitShader = VK_SHADER_UNUSED_KHR,
                .intersectionShader = VK_SHADER_UNUSED_KHR
            };
            shaderGroups.push_back(raygenGroup);
        // }

        // Miss shader
        // {
            shaderStages.push_back(shader.createShaderStageInfo(ctx.logicalDevice, "shaders/miss.spv", vk::ShaderStageFlagBits::eMissKHR));
            vk::RayTracingShaderGroupCreateInfoKHR missGroup
            {
                .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
                .generalShader = 1, // second entry in shaderStages
                .closestHitShader = VK_SHADER_UNUSED_KHR,
                .anyHitShader = VK_SHADER_UNUSED_KHR,
                .intersectionShader = VK_SHADER_UNUSED_KHR
            };
            shaderGroups.push_back(missGroup);
        // }

        // Closest hit shader
        // {
            shaderStages.push_back(shader.createShaderStageInfo(ctx.logicalDevice, "shaders/closesthit.spv", vk::ShaderStageFlagBits::eClosestHitKHR));
            vk::RayTracingShaderGroupCreateInfoKHR hitGroup
            {
                .type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
                .generalShader = VK_SHADER_UNUSED_KHR,
                .closestHitShader = 2, // third entry in shaderStages
                .anyHitShader = VK_SHADER_UNUSED_KHR,
                .intersectionShader = VK_SHADER_UNUSED_KHR
            };
            shaderGroups.push_back(hitGroup);
        // }
        vk::RayTracingPipelineCreateInfoKHR pipelineInfo
        {
            .pNext = nullptr,
            .flags = {},
            .stageCount = static_cast<uint32_t>(shaderStages.size()),
            .pStages = shaderStages.data(),
            .groupCount = static_cast<uint32_t>(shaderGroups.size()),
            .pGroups = shaderGroups.data(),
            .maxPipelineRayRecursionDepth = 1,
            .pLibraryInfo = nullptr,
            .pLibraryInterface = nullptr,
            .pDynamicState = nullptr,
            .layout = rayTracingPipelineLayout,
            .basePipelineHandle = nullptr,
            .basePipelineIndex = -1
        };
        rayTracingPipeline = ctx.logicalDevice.createRayTracingPipelineKHR(nullptr, nullptr, pipelineInfo, nullptr);
    }

    void RTRenderer::createShaderBindingTable()
    {
        VRTR_DEBUG("Creating Shader Binding Table");
        const uint32_t handle_size = rayTracingPipelineProperties.shaderGroupHandleSize; // rozmiar jednego shader group
        const uint32_t handle_alignment = rayTracingPipelineProperties.shaderGroupHandleAlignment;
        const uint32_t handle_size_aligned = aligned_size(handle_size, handle_alignment); // rozmiar wyrownania
        const uint32_t group_count = static_cast<uint32_t>(shaderGroups.size()); // licza shaderow
        const uint32_t sbt_size = group_count * handle_size_aligned; // calkowity rozmiar SBT - ile bajtow potrzeba zeby zmieniscic wszystkie uchryty shaderow
        const vk::BufferUsageFlags sbt_buffer_usage_flags = vk::BufferUsageFlagBits::eShaderBindingTableKHR | 
                                                            vk::BufferUsageFlagBits::eTransferSrc | 
                                                            vk::BufferUsageFlagBits::eShaderDeviceAddress;
        const vk::MemoryPropertyFlags sbt_memory_property_flags = vk::MemoryPropertyFlagBits::eHostVisible | 
                                                                  vk::MemoryPropertyFlagBits::eHostCoherent;
        
        raygen_shader_binding_table = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, handle_size, sbt_buffer_usage_flags, sbt_memory_property_flags);
        miss_shader_binding_table = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, handle_size, sbt_buffer_usage_flags, sbt_memory_property_flags);
        hit_shader_binding_table = std::make_unique<Buffer>(ctx.logicalDevice, ctx.gpu, handle_size, sbt_buffer_usage_flags, sbt_memory_property_flags);
        std::vector<uint8_t> shader_handle_storage(sbt_size);
        shader_handle_storage = rayTracingPipeline.getRayTracingShaderGroupHandlesKHR<uint8_t>(0, group_count, sbt_size);

        // KOPIOWANIE DANYCH Z CPU DO GPU:
        // najpierw mapujemy pamiec, zeby uzyskac wskaznik do pamieciu CPU z ktorego 
        // Dane będą kopiowane do pamieci GPU
        // Potem kopiujemy dane do pamieci CPU
        // Na koniec odmapowujemy pamiec
        // Bawimy sie handle_size_aligned, zeby uzyskiwac konrketne bajty w pamieci CPU,
        // kazdy shader jest zapisany w odpowieniej, stałej odleglosci od poczatku pamieci CPU - handle_size_aligned
        // unmap - odmapowanie pamieci - explicit zakończenie kopiowania danych

        // RAYGEN shader
        uint8_t *data = static_cast<uint8_t*>(raygen_shader_binding_table->map(handle_size, 0));
        memcpy(data, shader_handle_storage.data(), handle_size);
        raygen_shader_binding_table->unmap();
        // MISS shader
        data = static_cast<uint8_t*>(miss_shader_binding_table->map(handle_size, 0));
        memcpy(data, shader_handle_storage.data() + handle_size_aligned, handle_size);
        miss_shader_binding_table->unmap();

        // HIT shader
        data = static_cast<uint8_t*>(hit_shader_binding_table->map(handle_size, 0));
        memcpy(data, shader_handle_storage.data() + 2 * handle_size_aligned, handle_size);
        hit_shader_binding_table->unmap();
    }

    void RTRenderer::buildRTCommandBuffers()
    {
        // TODO dodać zmiane rozmiaru okienka!!!

        vk::CommandBufferBeginInfo beginInfo
        {
            .flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse,
            .pInheritanceInfo = nullptr
        };

        // vk::ImageSubresourceRange subresourceRange
        // {
        //     .aspectMask = vk::ImageAspectFlagBits::eColor,
        //     .baseMipLevel = 0,
        //     .levelCount = 1,
        //     .baseArrayLayer = 0,
        //     .layerCount = 1
        // };

        for(int32_t i = 0; i < ctx.commandBuffers.size(); i++)
        {
            ctx.commandBuffers.at(i).begin(beginInfo);

            const uint32_t handle_size = rayTracingPipelineProperties.shaderGroupHandleSize;
            const uint32_t handle_alignment = rayTracingPipelineProperties.shaderGroupHandleAlignment;
            const uint32_t handle_size_aligned = aligned_size(handle_size, handle_alignment);

            vk::StridedDeviceAddressRegionKHR raygenShaderSBTEntry
            {
                .deviceAddress = raygen_shader_binding_table->getDeviceAddress(),
                .stride = handle_size_aligned,
                .size = handle_size_aligned
            };

            vk::StridedDeviceAddressRegionKHR missShaderSBTEntry
            {
                .deviceAddress = miss_shader_binding_table->getDeviceAddress(),
                .stride = handle_size_aligned,
                .size = handle_size_aligned
            };

            vk::StridedDeviceAddressRegionKHR hitShaderSBTEntry
            {
                .deviceAddress = hit_shader_binding_table->getDeviceAddress(),
                .stride = handle_size_aligned,
                .size = handle_size_aligned
            };

            vk::StridedDeviceAddressRegionKHR callableShaderSBTEntry{};

            ctx.commandBuffers.at(i).bindPipeline(
                vk::PipelineBindPoint::eRayTracingKHR, 
                rayTracingPipeline
            );

            ctx.commandBuffers.at(i).bindDescriptorSets(
                vk::PipelineBindPoint::eRayTracingKHR, 
                *rayTracingPipelineLayout, 
                0, 
                {*descriptorSet}, 
                {}
            );

            VRTR_CommandBuffer->transition_image_layout
            (
                ctx.commandBuffers.at(i),
                storageImage.image, 
                vk::ImageLayout::eUndefined, 
                vk::ImageLayout::eGeneral,
                {},
                vk::AccessFlagBits2::eShaderWrite,
                vk::PipelineStageFlagBits2::eTopOfPipe,
                vk::PipelineStageFlagBits2::eRayTracingShaderKHR
            );

            ctx.commandBuffers.at(i).traceRaysKHR(
                raygenShaderSBTEntry,
                missShaderSBTEntry,
                hitShaderSBTEntry,
                callableShaderSBTEntry,
                width,
                height,
                1
            );

            VRTR_CommandBuffer->transition_image_layout
            (
                ctx.commandBuffers.at(i),
                ctx.swapChainImages.at(i), 
                vk::ImageLayout::eUndefined, 
                vk::ImageLayout::eTransferDstOptimal, // to jest potrzebne do kopiowania z storage image do swapchain image
                {},
                {},
                {}, 
                {}
            );

            VRTR_CommandBuffer->transition_image_layout
            (
                ctx.commandBuffers.at(i),
                storageImage.image, 
                vk::ImageLayout::eGeneral, 
                vk::ImageLayout::eTransferSrcOptimal, // to jest potrzebne do kopiowania z storage image do swapchain image
                {}, 
                vk::AccessFlagBits2::eTransferRead,
                vk::PipelineStageFlagBits2::eAllCommands,
                vk::PipelineStageFlagBits2::eTransfer
            );

            vk::ImageCopy copyRegion
            {
                .srcSubresource = vk::ImageSubresourceLayers{
                    vk::ImageAspectFlagBits::eColor,
                    0, 0, 1
                },
                .srcOffset = vk::Offset3D{0, 0, 0},
                .dstSubresource = vk::ImageSubresourceLayers{
                    vk::ImageAspectFlagBits::eColor,
                    0, 0, 1
                },
                .dstOffset = vk::Offset3D{0, 0, 0},
                .extent = vk::Extent3D{storageImage.width, storageImage.height, 1}
            };

            ctx.commandBuffers.at(i).copyImage(
                *storageImage.image, vk::ImageLayout::eTransferSrcOptimal,
                ctx.swapChainImages.at(i), vk::ImageLayout::eTransferDstOptimal,
                {copyRegion}
            );

            VRTR_CommandBuffer->transition_image_layout
            (
                ctx.commandBuffers.at(i),
                ctx.swapChainImages.at(i), 
                vk::ImageLayout::eTransferDstOptimal, 
                vk::ImageLayout::ePresentSrcKHR,
                {},
                {},
                {},
                {}
            );
            ctx.commandBuffers.at(i).end();
        }
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

            // recordCommandBuffer(imageIndex);
            ctx.logicalDevice.resetFences({ctx.drawFences[currentFrame]});

            vk::PipelineStageFlags waitDestinationStageMask( vk::PipelineStageFlagBits::eAllCommands );
            const vk::SubmitInfo submitInfo
            {
                .pNext = nullptr,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &*ctx.presentCompleteSemaphores.at(semaphoreIndex),
                .pWaitDstStageMask = &waitDestinationStageMask,
                .commandBufferCount = 1,
                .pCommandBuffers = &*ctx.commandBuffers.at(imageIndex),
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
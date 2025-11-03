#pragma once

#include "renderer_export.h"
#include "SwapChainManager.hpp"
#include "RasterGraphicsPipeline.hpp"
#include "CommandBuffer.hpp"
#include "ConstantsAndStructs.hpp"
#include "buffer.hpp"
#include "Camera.hpp"
#include "AccelerationStructureUtils.hpp"
#include "Utils.hpp"

namespace VRTR
{
#ifndef NDEBUG
    constexpr bool enableValidationLayers = true;
#else
    constexpr bool enableValidationLayers = false;
#endif

    class RENDERER_EXPORT RTRenderer
    {
    public:
        bool framebufferResized = false;

        RTRenderer() = default;
        ~RTRenderer();
        void init(GLFWwindow *window);
        void drawFrame(GLFWwindow *window);

    private:
        inline static std::vector<const char *> deviceExtensions // gpu logical device extensions
            {
                vk::KHRSwapchainExtensionName,
                vk::KHRSpirv14ExtensionName,
                vk::KHRRayQueryExtensionName,

                vk::KHRAccelerationStructureExtensionName,
                vk::KHRRayTracingPipelineExtensionName,
                vk::KHRDeferredHostOperationsExtensionName};

        inline static std::vector<const char *> validationLayers{
            "VK_LAYER_KHRONOS_validation"};

        inline static const std::vector<Vertex> vertices = {
            {{0.0f, -0.5f}, {1.0f, 0.2f, 0.0f}},
            {{0.5f, 0.5f}, {0.3f, 1.0f, 0.0f}},
            {{-0.5f, 0.5f}, {0.0f, 0.8f, 1.0f}}};

        std::vector<const char *> getRequiredExtensions();

        // Check if selected extensions are supported by the instance
        bool checkExtensionsSupport(const std::vector<const char *> &glfwExtensions,
                                    const std::vector<vk::ExtensionProperties> &extensionsProperties);

        void initInstance();

        void initValidationLayers();
        vk::DebugUtilsMessengerCreateInfoEXT populateDebugMessengerCreateInfo();

        bool isDeviceSuitable(const vk::raii::PhysicalDevice &device);

        uint32_t findQueueFamilies();

        void initPhysicalDeviceAndSurface(GLFWwindow *window);

        void initLogicalDevice();

        void initSwapChain(GLFWwindow *window);

        uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

        void initCommandBuffer();

        void createSyncObjects();

        // ================== RAY TRACING ==================

        void initRayTracing();

        void createBLAS();

        void createTLAS();

        void createScene();

        void createStorageImage();

        void createDescriptorSets();

        void updateDescriptorSets();

        void createRayTracingPipeline();

        void createShaderBindingTable();

        void buildRTCommandBuffers();

    private:
        int width, height;
        VULKAN_CONTEXT ctx;
        std::unique_ptr<SwapChainManager> swapChainManager;
        SurfaceCapabilities surfaceCapabilities;

        std::unique_ptr<SwapChainManager> VRTR_SwapChain;
        std::unique_ptr<RasterGraphicsPipeline> VRTR_RasterGraphicsPipeline;
        std::unique_ptr<CommandBuffer> VRTR_CommandBuffer;

        std::unique_ptr<Camera> camera;

        // ================== RAY TRACING ==================
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{};
        vk::raii::Pipeline rayTracingPipeline{nullptr};
        vk::raii::PipelineLayout rayTracingPipelineLayout{nullptr};

        // std::unique_ptr<Buffer> vertex_buffer;
        // std::unique_ptr<Buffer> index_buffer;
        std::shared_ptr<primitiveBuffers> primitive_buffers;
        std::unique_ptr<Buffer> uniform_buffer;
        UniformData uniform_data{};
        AccelerationStructure blas_structure;
        AccelerationStructure tlas_structure;

        StorageImage storageImage;
        // DESCRIPTOR SETS
        vk::raii::DescriptorPool descriptorPool{nullptr};
        vk::raii::DescriptorSet descriptorSet{nullptr};
        vk::raii::DescriptorSetLayout descriptorSetLayout{nullptr};

        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups{};
        // Shader Binding Table
        std::unique_ptr<Buffer> raygen_shader_binding_table;
        std::unique_ptr<Buffer> miss_shader_binding_table;
        std::unique_ptr<Buffer> hit_shader_binding_table;

        void updateUniformBuffer();
    };

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                          vk::DebugUtilsMessageTypeFlagsEXT type,
                                                          const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                          void *);
    inline static void framebufferResizeCallback(GLFWwindow *window, int width, int height)
    {
        auto app = reinterpret_cast<RTRenderer *>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }
}
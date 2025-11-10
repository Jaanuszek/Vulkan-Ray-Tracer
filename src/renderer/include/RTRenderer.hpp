#pragma once

#include "renderer_export.h"
#include "SwapChainManager.hpp"
#include "CommandBuffer.hpp"
#include "ConstantsAndStructs.hpp"
#include "RendererContext.hpp"
#include "InstanceManager.hpp"
#include "buffer.hpp"
#include "Camera.hpp"
#include "AccelerationStructureUtils.hpp"
#include "Shader.hpp"
#include "Utils.hpp"

namespace VRTR
{
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
                vk::KHRDeferredHostOperationsExtensionName
            };

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
        // TODO replace VULKAN_CONTEXT with RendererContext
        RendererContext rendererContext;

        std::unique_ptr<SwapChainManager> swapChainManager;
        SurfaceCapabilities surfaceCapabilities;

        std::unique_ptr<SwapChainManager> VRTR_SwapChain;
        std::unique_ptr<CommandBuffer> VRTR_CommandBuffer;

        std::unique_ptr<Camera> camera;

        // ================== RAY TRACING ==================
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{};
        vk::raii::Pipeline rayTracingPipeline{nullptr};
        vk::raii::PipelineLayout rayTracingPipelineLayout{nullptr};

        std::unique_ptr<Buffer> vertex_buffer;
        std::unique_ptr<Buffer> index_buffer;
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

    inline static void framebufferResizeCallback(GLFWwindow *window, int width, int height)
    {
        auto app = reinterpret_cast<RTRenderer *>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }
}
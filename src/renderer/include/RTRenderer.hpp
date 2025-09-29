#pragma once

#include "renderer_export.h"
#include "SwapChainManager.hpp"
#include "RasterGraphicsPipeline.hpp"
#include "CommandBuffer.hpp"
#include "ConstantsAndStructs.hpp"
#include "buffer.hpp"
#include "Camera.hpp"

namespace VRTR
{
    #ifndef NDEBUG
        constexpr bool enableValidationLayers = true;
        std::vector<const char*> validationLayers
        {
            "VK_LAYER_KHRONOS_validation"
        };
    #elif
        constexpr bool enableValidationLayers = false;
    #endif

    class RENDERER_EXPORT RTRenderer
    {
        public:
            bool framebufferResized = false;

            RTRenderer() = default;
            ~RTRenderer();
            void init(GLFWwindow* window);
            void drawFrame(GLFWwindow* window);

        private:

            inline static std::vector<const char*> deviceExtensions // gpu logical device extensions
            {
                vk::KHRSwapchainExtensionName,
                vk::KHRSpirv14ExtensionName,
                vk::KHRRayQueryExtensionName,

                vk::KHRAccelerationStructureExtensionName,
                vk::KHRRayTracingPipelineExtensionName,
                vk::KHRDeferredHostOperationsExtensionName
            };

            inline static const std::vector<Vertex> vertices = {
                {{0.0f, -0.5f}, {1.0f, 0.2f, 0.0f}},
                {{0.5f, 0.5f}, {0.3f, 1.0f, 0.0f}},
                {{-0.5f, 0.5f}, {0.0f, 0.8f, 1.0f}}
            };

            std::vector<const char*> getRequiredExtensions();

            // Check if selected extensions are supported by the instance
            bool checkExtensionsSupport(const std::vector<const char*>& glfwExtensions, 
                                         const std::vector<vk::ExtensionProperties>& extensionsProperties);

            void initInstance();

            #ifndef NDEBUG
                void initValidationLayers();
                vk::DebugUtilsMessengerCreateInfoEXT populateDebugMessengerCreateInfo();
            #endif

            bool isDeviceSuitable(const vk::raii::PhysicalDevice& device);

            uint32_t findQueueFamilies();

            void initPhysicalDeviceAndSurface(GLFWwindow* window);

            void initLogicalDevice();

            void initSwapChain(GLFWwindow* window);

            uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

            void initPipeline();

            void initCommandBuffer();

            void createSyncObjects();

            void recordCommandBuffer(uint32_t imageIndex);

            // ================== RAY TRACING ==================

            void initRayTracing();

            ScratchBuffer createScratchBuffer(vk::DeviceSize size);

            void createBLAS();

            void createTLAS();

            void createScene();

            void createStorageImage();

            void createDescriptorSets();

            void createRayTracingPipeline();

            void createShaderBindingTable();

            void buildRTCommandBuffers();

        private:
            int width, height;
            Context ctx;
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


    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                vk::DebugUtilsMessageTypeFlagsEXT type,
                                                const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                void*);
    inline static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<RTRenderer*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    // zaokrągla value do najbliższej wielokrotności alignment
    // value + alignment - 1 zaokrągla w górę
    // ~(alignment - 1) neguje bity alignment -1 przez co zerująy się bity mniejsze niż alignment -1
    // operacja & zostawia tylko bity większe lub równe alignment
    // np. value = 13 alignment = 8
    // 13 + 8 - 1 = 20 = 00010100
    // 8 - 1 = 7 = 00000111
    // ~7 = 11111000
    // 20 & 11111000 = 16 = 00010000
    // wynik to 16 czyli najbliższa wielokrotność 8 większa lub równa 13
    inline uint32_t aligned_size(uint32_t value, uint32_t alignment)
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }
}
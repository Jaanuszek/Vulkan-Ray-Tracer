#pragma once

#include "renderer_export.h"
#include "SwapChainManager.hpp"
#include "CommandBufferManager.hpp"
#include "ConstantsAndStructs.hpp"
// #include "RendererContext.hpp"
#include "InstanceManager.hpp"
#include "DeviceManager.hpp"
#include "buffer.hpp"
#include "Camera.hpp"
#include "Shader.hpp"
#include "Utils.hpp"
#include "StorageImage.hpp"
#include "AccelerationStructureManager.hpp"
#include "DescriptorManager.hpp"
#include "RayTracingPipeline.hpp"

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
        uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

        void createSyncObjects();

        glm::mat4 rotateModel(float angle, const glm::vec3 &axis);

        void createScene();

    private:
        int width, height;
        VULKAN_CONTEXT ctx;
        // TODO replace VULKAN_CONTEXT with RendererContext
        // RendererContext rendererContext;

        std::unique_ptr<SwapChainManager> swapChainManager;
        std::shared_ptr<CommandBufferManager> commandBufferManager;
        std::shared_ptr<StorageImage> storageImage;
        std::unique_ptr<AccelerationStructureManager> asManager;
        std::unique_ptr<RayTracingPipeline> rayTracingPipeline;

        std::unique_ptr<Camera> camera;

        // ================== RAY TRACING ==================
        std::unique_ptr<Buffer> uniform_buffer;
        UniformData uniform_data{};

        void updateUniformBuffer();
    };

    inline static void framebufferResizeCallback(GLFWwindow *window, int width, int height)
    {
        auto app = reinterpret_cast<RTRenderer *>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }
}
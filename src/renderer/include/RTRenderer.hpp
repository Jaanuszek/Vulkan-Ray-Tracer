#pragma once

#include "VmaUsage.h"

#include "renderer_export.h"
#include "SwapChainManager.hpp"
#include "CommandBufferManager.hpp"
#include "ConstantsAndStructs.hpp"
#include "SceneSettings.hpp"
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
#include "Model.hpp"
#include "Texture.hpp"
#include "StorageBuffer.hpp"
#include "GUI.hpp"
#include "init_cuda.cuh"
#include "defines.hpp"
#include "Scene.hpp"
#include "vkCudaInterop.hpp"

namespace VRTR
{
    class RENDERER_EXPORT RTRenderer
    {
    public:
        bool framebufferResized = false;

        RTRenderer(std::shared_ptr<Camera> camera, SceneSettings &sceneSettings);
        ~RTRenderer();
        void init(GLFWwindow *window);
        void drawFrame(GLFWwindow *window, double deltaTime, bool renderGUI);

    private:
        void setupVMA();

        void setupCuda();

        void initImGUI(GLFWwindow* window);

        void createSyncObjects();

        void createExternalSemaphore(vk::ExternalSemaphoreHandleTypeFlagBits handleType);

        void createScene();

        void recreateResources(GLFWwindow *window);

        std::pair<std::vector<VertexRT>, std::vector<uint32_t>> createFloor();

        void updateUniformBuffer();

    private:
        int width, height;
        uint64_t frameCount{};
        RendererContext ctx;

        std::unique_ptr<GUI> gui;

        std::unique_ptr<SwapChainManager> swapChainManager;
        std::shared_ptr<CommandBufferManager> commandBufferManager;
        std::shared_ptr<StorageImage> storageImage;
        std::unique_ptr<Scene> scene;
        std::unique_ptr<RayTracingPipeline> rayTracingPipeline;
        std::unique_ptr<vkCudaInterop> vkCudaInteop;

        std::shared_ptr<Camera> camera;

        // internal semaphores and fences for synchronisation
        std::vector<vk::raii::Semaphore> presentCompleteSemaphores;
        std::vector<vk::raii::Semaphore> renderCompleteSemaphores;
        std::vector<vk::raii::Fence> drawFences;

        // ================== RAY TRACING ==================
        std::unique_ptr<Buffer> uniform_buffer;
        SceneSettings &sceneSettings;
    };

    inline static void framebufferResizeCallback(GLFWwindow *window, int width, int height)
    {
        auto app = reinterpret_cast<RTRenderer *>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }
}
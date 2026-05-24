#include <pch.h>

#include "Logger.hpp"
#include "InputManager.hpp"
#include "Camera.hpp"
#include "ConstantsAndStructs.hpp"
#include "RTRenderer.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "init_cuda.cuh"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
static vk::detail::DynamicLoader dl;

// Start resolution
// On window resize, those values will not be updated here, but inside RTRenderer
constexpr unsigned int WIDTH = 800;
constexpr unsigned int HEIGHT = 600;

double lastFrameTime{};
double deltaTime{};

constexpr uint32_t FrameToCapture = 50000;

int main()
{
    try
    {
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        VRTR::Logger::init();
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
        GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "VRTR", nullptr, nullptr);

        SceneSettings sceneSettings{};

        std::shared_ptr<VRTR::Camera> camera = std::make_shared<VRTR::Camera>(sceneSettings, glm::vec3(0.0f, 0.0f, 4.0f));
        camera->setPerspective(60.0f, static_cast<float>(WIDTH) / HEIGHT, 0.1f, 100.0f);

        std::unique_ptr<VRTR::RTRenderer> renderer = std::make_unique<VRTR::RTRenderer>(camera, sceneSettings);
        renderer->init(window);

        glfwSetWindowUserPointer(window, camera.get());
        glfwSetFramebufferSizeCallback(window, VRTR::framebufferResizeCallback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        VRTR::InputManager::init(window);
        
        uint32_t frameCounter = 0;
        std::array<float, FrameToCapture> frameTimes{};

        while(!glfwWindowShouldClose(window)) {
            double currentTime = glfwGetTime();
            deltaTime = currentTime - lastFrameTime;
            lastFrameTime = currentTime;
            glfwPollEvents();
            VRTR::processInput(window, deltaTime, camera);
            renderer->drawFrame(window, deltaTime, VRTR::InputManager::renderGUI);
            if(frameCounter < FrameToCapture)
            {
                frameTimes[frameCounter % FrameToCapture] = static_cast<float>(deltaTime);
            }
            // else
            // {
                // VRTR_INFO("CAPTURED ALL FRAMES");
                // break;
            // }
            frameCounter++;
        }

        double sum = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0);
        double averageFrameTime = sum / std::min(frameCounter, FrameToCapture);
        VRTR_INFO("Average frame time {} ms", averageFrameTime * 1000.0);

        renderer.reset();   
        glfwDestroyWindow(window);

        glfwTerminate();
    }
    catch (const std::exception& e)
    {
        VRTR_CRITICAL("Exception: {}", e.what());
        return EXIT_FAILURE;
    }

    return 0;
}

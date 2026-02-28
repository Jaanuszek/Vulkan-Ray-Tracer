#include <pch.h>

#include "Logger.hpp"
#include "InputManager.hpp"
#include "Camera.hpp"
#include "ConstantsAndStructs.hpp"
#include "RTRenderer.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
static vk::detail::DynamicLoader dl;

// Start resolution
// On window resize, those values will not be updated here, but inside RTRenderer
constexpr unsigned int WIDTH = 800;
constexpr unsigned int HEIGHT = 600;

double lastFrameTime{};
double deltaTime{};

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
        GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Siema Eniu", nullptr, nullptr);

        std::shared_ptr<VRTR::Camera> camera = std::make_shared<VRTR::Camera>(glm::vec3(0.0f, 0.0f, 4.0f));
        camera->setPerspective(45.0f, static_cast<float>(WIDTH) / HEIGHT, 0.1f, 100.0f);

        std::unique_ptr<VRTR::RTRenderer> renderer = std::make_unique<VRTR::RTRenderer>(camera);
        renderer->init(window);

        glfwSetWindowUserPointer(window, camera.get());
        glfwSetFramebufferSizeCallback(window, VRTR::framebufferResizeCallback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        VRTR::InputManager::init(window);
        
        while(!glfwWindowShouldClose(window)) {
            double currentTime = glfwGetTime();
            deltaTime = currentTime - lastFrameTime;
            lastFrameTime = currentTime;
            glfwPollEvents();
            VRTR::processInput(window, deltaTime, camera);
            renderer->drawFrame(window, deltaTime);
        }

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

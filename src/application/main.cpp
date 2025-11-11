#include <pch.h>

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"
#include "RTRenderer.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
static vk::detail::DynamicLoader dl;

constexpr unsigned int WIDTH = 800;
constexpr unsigned int HEIGHT = 600;

int main()
{
    try
    {
        // #if defined(_HPP_VULKAN_LIBRARY)
        //         static vk::detail::DynamicLoader dl(_HPP_VULKAN_LIBRARY);
        // #else
        //         static vk::detail::DynamicLoader dl;
        // #endif
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        VRTR::Logger::init();
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Siema Eniu", nullptr, nullptr);

        std::unique_ptr<VRTR::RTRenderer> renderer = std::make_unique<VRTR::RTRenderer>();
        renderer->init(window);
        glfwSetWindowUserPointer(window, renderer.get());
        glfwSetFramebufferSizeCallback(window, VRTR::framebufferResizeCallback);
        
        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            renderer->drawFrame(window);
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

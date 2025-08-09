#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <pch.h>

#include "Logger.hpp"
#include "RTRenderer.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

constexpr unsigned int WIDTH = 800;
constexpr unsigned int HEIGHT = 600;

int main()
{
    try
    {
        VRTR::Logger::init();
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Siema Eniu", nullptr, nullptr);

        std::unique_ptr<VRTR::RTRenderer> renderer = std::make_unique<VRTR::RTRenderer>();
        renderer->init();
        
        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();   
        }
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

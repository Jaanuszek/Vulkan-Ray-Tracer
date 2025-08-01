#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <pch.h>

#include "Logger.hpp"
#include "RTRenderer.hpp"

int main()
{
    VRTR::Logger::init();
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Siema Eniu", nullptr, nullptr);
    VRTR::RTRenderer renderer;
    renderer.init();
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
    renderer.destroy();
    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}

#include "pch.h"
#include "WindowSurface.hpp"

namespace VRTR
{
    void WindowSurface::setupSurface(vk::raii::Instance& inst, GLFWwindow* window, vk::raii::SurfaceKHR& sur)
    {
        VRTR_DEBUG("INITIALIZING WINDOW SURFACE");

        VkSurfaceKHR tempSurface;
        if(glfwCreateWindowSurface(*inst, window, nullptr, &tempSurface) != VK_SUCCESS)
        {
            VRTR_ERROR("Failed to create window surface");
            throw std::runtime_error("Failed to create window surface");
        }
        sur = vk::raii::SurfaceKHR(inst, tempSurface);
    }
}

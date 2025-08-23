#pragma once
#include "Logger.hpp"

namespace VRTR
{
// DO I NEED THIS CLASS?
    class WindowSurface
    {
        public:
            WindowSurface() = default;
            ~WindowSurface() = default;

            void setupSurface(vk::raii::Instance& inst, GLFWwindow* window, vk::raii::SurfaceKHR& sur);

        private:
    };
}
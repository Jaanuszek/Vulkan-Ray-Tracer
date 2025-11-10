#pragma once

#include "Logger.hpp"
#include "pch.h"

namespace VRTR
{
    struct RendererContext
    {
        vk::raii::Context context;
        vk::raii::Instance instance{nullptr};
        vk::raii::PhysicalDevice physicalDevice{nullptr};
        vk::raii::Device device{nullptr};
        vk::raii::Queue queue{nullptr};
        vk::raii::SurfaceKHR surface{nullptr};
        int32_t graphics_queue_index = -1;

        vk::raii::DebugUtilsMessengerEXT debugMessenger{nullptr};
    };
}
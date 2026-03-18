#pragma once
#include "pch.h"
#include "VmaUsage.h"

namespace VRTR
{
    class Buffer;

    struct Properties
    {
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtPipelineProperties{};
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR asProperties{};
    };

    struct RendererContext
    {
        vk::raii::Context context;
        vk::raii::Instance instance{nullptr};
        vk::raii::PhysicalDevice gpu{nullptr};
        vk::raii::Device logicalDevice{nullptr};
        vk::raii::Queue queue{nullptr};
        int32_t graphics_queue_index = -1;
        vk::raii::SurfaceKHR surface{nullptr};
        vk::raii::DebugUtilsMessengerEXT debugMessenger{nullptr};
        VmaAllocator vmaAllocator{nullptr};
        Properties properties;
    };

    struct VertexRT
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 texCoord;
    };

    namespace CONSTANTS
    {
        inline std::filesystem::path getExecutableDir()
        {
#ifdef _WIN32
            char buffer[MAX_PATH];
            GetModuleFileNameA(nullptr, buffer, MAX_PATH);
            return std::filesystem::path(buffer).parent_path();
#else
            char buffer[1024];
            ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer));
            return std::filesystem::path(std::string(buffer, (count > 0) ? count : 0)).parent_path();
#endif
        }

        const std::filesystem::path EXEC_DIR = getExecutableDir();
        const std::filesystem::path SHADERS_DIR = EXEC_DIR / "../shaders";
        const std::filesystem::path ASSETS_DIR = EXEC_DIR / "../assets";
        
        constexpr int MAX_FRAMES_IN_FLIGHT = 3;

        constexpr uint32_t MAX_OBJECTS = 1024;
        constexpr uint32_t MAX_TEXTURES = 16;
    }
}
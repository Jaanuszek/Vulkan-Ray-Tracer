#pragma once
#include "pch.h"

namespace VRTR
{
    class Buffer;

    struct VULKAN_CONTEXT
    {
        vk::raii::Context context;

        vk::raii::Instance instance{nullptr};

        vk::raii::PhysicalDevice gpu{nullptr};

        vk::raii::Device logicalDevice{nullptr};

        vk::raii::Queue queue{nullptr};

        int32_t graphics_queue_index = -1;

        vk::raii::SurfaceKHR surface{nullptr};

        vk::raii::CommandPool commandPool{nullptr};

        std::vector<vk::raii::CommandBuffer> commandBuffers;

        vk::raii::DebugUtilsMessengerEXT debugMessenger{nullptr};

        // SYNC VARIABLES
        std::vector<vk::raii::Semaphore> presentCompleteSemaphores;

        std::vector<vk::raii::Semaphore> renderCompleteSemaphores;

        std::vector<vk::raii::Fence> drawFences;

        // CONST VALUES
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtPipelineProperties{};
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR asProperties{};
    };

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

        Properties properties;
    };

    struct Vertex
    {
        glm::vec2 pos;
        glm::vec3 color;
    };

    struct VertexRT
    {
        glm::vec3 pos;
    };

    struct UniformData
    {
        glm::mat4 view_inverse;
        glm::mat4 proj_inverse;
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
    }

}
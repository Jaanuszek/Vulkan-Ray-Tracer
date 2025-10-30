#pragma once
#include "pch.h"

namespace VRTR
{
    constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    inline uint32_t currentFrame = 0;
    inline uint32_t semaphoreIndex = 0;

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

        vk::raii::SwapchainKHR swapChain{nullptr};
        
        std::vector<vk::Image> swapChainImages;

        std::vector<vk::raii::ImageView> swapChainImageViews;

        vk::raii::PipelineLayout pipelineLayout{nullptr};

        vk::raii::Pipeline pipeline{nullptr};

        vk::raii::CommandPool commandPool{nullptr};

        std::vector<vk::raii::CommandBuffer> commandBuffers;

        vk::DebugUtilsMessengerEXT debugMessenger{nullptr};

        // vk::raii::Buffer vertex_buffer{nullptr};
        std::unique_ptr<Buffer> vertex_buffer;
        
        vk::raii::DeviceMemory vertex_buffer_memory{nullptr};

        // SYNC VARIABLES
        std::vector<vk::raii::Semaphore> presentCompleteSemaphores;

        std::vector<vk::raii::Semaphore> renderCompleteSemaphores;

        std::vector<vk::raii::Fence> drawFences;

        // CONST VALUES
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtPipelineProperties{};
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR asProperties{};
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

    struct primitiveBuffers
    {
        std::unique_ptr<Buffer> vertexBuffer;
        std::unique_ptr<Buffer> indexBuffer;
    };

    struct UniformData
    {
        glm::mat4 view_inverse;
        glm::mat4 proj_inverse;
    };

    struct AccelerationStructure
    {
        std::unique_ptr<Buffer> buffer; // to raczej niepotrzebne
        vk::raii::AccelerationStructureKHR handle{nullptr};
        vk::DeviceAddress device_address;
    };

    struct StorageImage
    {
        uint32_t width;
        uint32_t height;
        vk::raii::Image image{nullptr};
        vk::raii::ImageView imageView{nullptr};
        vk::raii::DeviceMemory memory{nullptr};
    };

    namespace CONSTANTS
    {
        inline std::filesystem::path getExecutableDir() {
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
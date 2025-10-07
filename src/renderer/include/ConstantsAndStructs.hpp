#pragma once
#include "pch.h"
#include "buffer.hpp"

namespace VRTR
{
    constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    inline uint32_t currentFrame = 0;
    inline uint32_t semaphoreIndex = 0;

    struct Context
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

    struct ScratchBuffer
    {
        vk::raii::Buffer buffer{nullptr};
        vk::raii::DeviceMemory memory{nullptr};
        uint64_t device_address;
    };

    struct AccelerationStructure
    {
        std::unique_ptr<Buffer> buffer;
        vk::raii::AccelerationStructureKHR handle{nullptr};
        vk::DeviceAddress device_address; // vk::DeviceAddress??
    };

    struct StorageImage
    {
        uint32_t width;
        uint32_t height;
        vk::raii::Image image{nullptr};
        vk::raii::ImageView imageView{nullptr};
        vk::raii::DeviceMemory memory{nullptr};
    };

    namespace utils
    {
        // zaokrągla value do najbliższej wielokrotności alignment
        // value + alignment - 1 zaokrągla w górę
        // ~(alignment - 1) neguje bity alignment -1 przez co zerująy się bity mniejsze niż alignment -1
        // operacja & zostawia tylko bity większe lub równe alignment
        // np. value = 13 alignment = 8
        // 13 + 8 - 1 = 20 = 00010100
        // 8 - 1 = 7 = 00000111
        // ~7 = 11111000
        // 20 & 11111000 = 16 = 00010000
        // wynik to 16 czyli najbliższa wielokrotność 8 większa lub równa 13
        inline uint32_t aligned_size(uint32_t value, uint32_t alignment)
        {
            return (value + alignment - 1) & ~(alignment - 1);
        }
    }
}
#pragma once

#include "Logger.hpp"
#include "buffer.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "VmaUsage.h"
#include "Image.hpp"
#include "ConstantsAndStructs.hpp"

namespace VRTR
{
    namespace GUIConstants
    {
        constexpr float styleScale = 1.5f;
    }

    class GUI
    {
    public:
        GUI(RendererContext& ctx);
        ~GUI();

        void init(GLFWwindow* window, float width, float height);
        void initResources(vk::raii::CommandPool& commandPool, vk::Format swapchainFormat, uint32_t imageCount);
        void setStyle(uint32_t index);
        void setupStyle();

        bool newFrame();
        vk::CommandBuffer buildDrawCommandBuffer(uint32_t imageIndex, vk::Image swapchainImage, vk::ImageView swapchainImageView, vk::Extent2D extent);

        void handleKey(int ket, int scancode, int action, int mods);
        bool getWantKeyCapture();
        void charPressed(uint32_t key);

    private:
        vk::raii::DescriptorPool descriptorPool{nullptr};
        std::vector<vk::raii::CommandBuffer> guiCommandBuffers;
        VkFormat swapchainFormat{VK_FORMAT_B8G8R8A8_UNORM};
        uint32_t imageCount{0};

        // vulkan context
        RendererContext& ctx;
        GLFWwindow* window{nullptr};

        // UI specific
        ImGuiStyle vulkanStyle;
        VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo{};
        float mainScale;
    };
}
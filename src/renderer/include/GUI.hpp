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
    class GUI
    {
    public:
        GUI(RendererContext& ctx);
        ~GUI();

        void init(float width, float height);
        void initResources();
        void setStyle(uint32_t index);

        bool newFrame();
        void updateBuffers();
        void drawFrame(vk::raii::CommandBuffer &commandBuffer);

        void handleKey(int ket, int scancode, int action, int mods);
        bool getWantKeyCapture();
        void charPressed(uint32_t key);

    private:
        vk::raii::Sampler sampler{nullptr};
        std::unique_ptr<Buffer> vertexBuffer;
        std::unique_ptr<Buffer> indexBuffer;
        uint32_t vertexCount{};
        uint32_t indexCount{};
        std::unique_ptr<Image> fontImage;

        // vulkan pipeline
        vk::raii::PipelineCache pipelineCache{nullptr};
        vk::raii::PipelineLayout pipelineLayout{nullptr};
        vk::raii::Pipeline pipeline{nullptr};
        vk::raii::DescriptorPool descriptorPool{nullptr};
        vk::raii::DescriptorSetLayout descriptorSetLayout{nullptr};
        vk::raii::DescriptorSet descriptorSet{nullptr};

        // vulkan context
        RendererContext& ctx;

        // UI specific
        ImGuiStyle vulkanStyle;
        struct PushConstBlock
        {
            glm::vec2 scale;
            glm::vec2 translate;
        } pushConstBlock;
        bool needsUpdateBuffers = false;
        vk::PipelineRenderingCreateInfo renderingInfo{};
        vk::Format colorFormat = vk::Format::eB8G8R8A8Uint;
    };
}
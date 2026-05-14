#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"
#include "Shader.hpp"
#include "CommonStructs.h"
#include "VisibilityDescriptorManager.hpp"
#include "Utils.hpp"

namespace VRTR
{
    constexpr uint32_t RAYS_PER_PATCH = 2048;
    class VisibilityPipeline
    {
    public:
        VisibilityPipeline(RendererContext& ctx);
        ~VisibilityPipeline();

        void init(DescriptorResources& resources, std::shared_ptr<CommandBufferManager>& commandBufferManager);

        void updatePipelineDescriptors(DescriptorResources& resources);

    private:
        void createPipelineLayout();
        void createShaderStages();
        void createPipeline();
        void createSBT();
        void recordCommandBuffer();

    private:
        RendererContext& ctx;
        vk::Buffer visibilityOutputBuffer{nullptr};
        Shader shader;

        std::unique_ptr<VisibilityDescriptorManager> descriptorManager;
        std::shared_ptr<CommandBufferManager> commandBufferManager;

        vk::raii::PipelineLayout pipelineLayout{nullptr};
        vk::raii::Pipeline pipeline{nullptr};

        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages{};
        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups{};

        std::unique_ptr<Buffer> raygen_shader_binding_table;
        std::unique_ptr<Buffer> miss_shader_binding_table;
        std::unique_ptr<Buffer> hit_shader_binding_table;
    };
}
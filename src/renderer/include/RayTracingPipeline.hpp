#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"
#include "buffer.hpp"
#include "Shader.hpp"
#include "Utils.hpp"
#include "CommandBufferManager.hpp"
#include "StorageImage.hpp"
#include "DescriptorManager.hpp"

namespace VRTR
{
    struct PushConstant
    {
        uint64_t vertices; //addr to vertex buffer
        uint64_t indices; //addr to index buffer
    };

    class RayTracingPipeline
    {
        public:
            RayTracingPipeline(RendererContext& ctx, PushConstant& pushConstantData);

            void init(std::vector<vk::Image>& swapChainImages,
                    const DescriptorResources& resources,
                    std::shared_ptr<CommandBufferManager>& commandBufferManager,
                    int width, int height,
                    std::shared_ptr<StorageImage>& storageImage
                );

            void updatePipelineDescriptors(const DescriptorResources& resources, int width, int height);

            static void initRayTracing(RendererContext &ctx);

        private:
            void createRayTracingPipeline();

            void createShaderBindingTable();

            void buildRTCommandBuffers();

        private:
            RendererContext& ctx;
            PushConstant pushConstantData;

            std::vector<vk::Image>* swapChainImages = nullptr;
            std::unique_ptr<DescriptorManager> descriptorManager;
            std::shared_ptr<CommandBufferManager> commandBufferManager;
            std::shared_ptr<StorageImage> storageImage;
            int width{0}; int height{0};
            // vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{};
            vk::raii::PipelineLayout rayTracingPipelineLayout{nullptr};
            vk::raii::Pipeline rayTracingPipeline{nullptr};

            std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups{};

            std::unique_ptr<Buffer> raygen_shader_binding_table;
            std::unique_ptr<Buffer> miss_shader_binding_table;
            std::unique_ptr<Buffer> hit_shader_binding_table;
    };
}
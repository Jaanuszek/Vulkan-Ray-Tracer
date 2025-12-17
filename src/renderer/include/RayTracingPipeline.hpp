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

    class RayTracingPipeline
    {
        public:
            RayTracingPipeline(VULKAN_CONTEXT& ctx);

            void init(std::shared_ptr<DescriptorManager>& descriptorManager,
                    std::vector<vk::Image>& swapChainImages,
                    std::shared_ptr<CommandBufferManager>& commandBufferManager,
                    int width, int height,
                    std::shared_ptr<StorageImage>& storageImage
                );

            void buildRTCommandBuffers(std::vector<vk::Image>& swapChainImages,
                        std::shared_ptr<CommandBufferManager>& commandBufferManager,
                        std::shared_ptr<DescriptorManager>& descriptorManager,
                        int width, int height,
                        std::shared_ptr<StorageImage>& storageImage);

            static void initRayTracing(VULKAN_CONTEXT &ctx);
            

        private:
            void createRayTracingPipeline(std::shared_ptr<DescriptorManager>& descriptorManager);

            void createShaderBindingTable();

        private:
            VULKAN_CONTEXT& ctx;
            // vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{};
            vk::raii::PipelineLayout rayTracingPipelineLayout{nullptr};
            vk::raii::Pipeline rayTracingPipeline{nullptr};

            std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups{};

            std::unique_ptr<Buffer> raygen_shader_binding_table;
            std::unique_ptr<Buffer> miss_shader_binding_table;
            std::unique_ptr<Buffer> hit_shader_binding_table;
    };
}
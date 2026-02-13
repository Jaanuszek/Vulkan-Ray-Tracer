#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"
#include "Texture.hpp"

namespace VRTR
{
    struct DescriptorResources
    {
        vk::raii::AccelerationStructureKHR* TLAS = nullptr;
        vk::raii::Buffer* ubo = nullptr;
        vk::raii::ImageView* storageImageView = nullptr;
        vk::raii::ImageView* texImageView = nullptr;
        vk::raii::Sampler* texSampler = nullptr;
    };

    class DescriptorManager
    {
        public:
            DescriptorManager(RendererContext& ctx);

            void init(const DescriptorResources& resources);

            void updateDescriptorSets();

            // gettery
            vk::raii::DescriptorPool & getDescriptorPool() { return descriptorPool; }
            vk::raii::DescriptorSet & getDescriptorSet() { return descriptorSet; }
            vk::raii::DescriptorSetLayout & getDescriptorSetLayout() { return descriptorSetLayout; }

            //settery
            void setDescriptorSetLayout(vk::raii::DescriptorSetLayout& layout) { descriptorSetLayout = std::move(layout); }
            void setDescriptorResources(const DescriptorResources& resources) { descriptorResources = resources; }

        private:
            void createDescriptorSets();

        private:
            RendererContext& ctx;
            DescriptorResources descriptorResources;

            vk::raii::DescriptorPool descriptorPool{nullptr};
            vk::raii::DescriptorSet descriptorSet{nullptr};
            vk::raii::DescriptorSetLayout descriptorSetLayout{nullptr};
    };
}
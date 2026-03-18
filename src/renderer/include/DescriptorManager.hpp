#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"
#include "Texture.hpp"
#include "DescriptorManager.hpp"

namespace VRTR
{
    struct DescriptorResources
    {
        // TODO zaimplementowac dirty flag design pattern tutaj
        vk::AccelerationStructureKHR TLAS = nullptr;
        vk::Buffer ubo = nullptr;
        vk::ImageView storageImageView = nullptr;
        std::vector<vk::ImageView> texImageViews;
        std::vector<vk::Sampler> texSamplers;
        vk::Buffer geometryInfoBuffer = nullptr;
        vk::Buffer materialBuffer = nullptr;
        vk::Buffer cudaColorBuffer = nullptr;
        vk::Buffer triToPatchBuffer = nullptr;
        vk::Buffer patchBuffer = nullptr;
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
            void createDescriptorSetLayout();
            void createDescriptorPool();
            void writeDescriptorSet();
            void allocateDescriptorSet();
            void createDescriptorSets();

        private:
            RendererContext& ctx;
            DescriptorResources descriptorResources;

            vk::raii::DescriptorPool descriptorPool{nullptr};
            vk::raii::DescriptorSet descriptorSet{nullptr};
            vk::raii::DescriptorSetLayout descriptorSetLayout{nullptr};
    };
}
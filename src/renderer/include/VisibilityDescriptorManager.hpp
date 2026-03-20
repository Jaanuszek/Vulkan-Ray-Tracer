#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"
#include "Texture.hpp"
#include "DescriptorManager.hpp"

namespace VRTR
{
    // TODO moze zrobic interfejs IDescriptorManager?
    class VisibilityDescriptorManager
    {
        public:
            VisibilityDescriptorManager(RendererContext& ctx);
            VisibilityDescriptorManager() = delete;

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
            void createDescriptorSetLayout();
            void createDescriptorPool();
            void allocateDescriptorSet();
            void writeDescriptorSet();

        private:
            RendererContext& ctx;
            DescriptorResources descriptorResources;

            vk::raii::DescriptorPool descriptorPool{nullptr};
            vk::raii::DescriptorSet descriptorSet{nullptr};
            vk::raii::DescriptorSetLayout descriptorSetLayout{nullptr};
    };
}
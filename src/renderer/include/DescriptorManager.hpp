#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"

namespace VRTR
{
    class DescriptorManager
    {
        public:
            DescriptorManager(VULKAN_CONTEXT& ctx);

            void init(vk::raii::AccelerationStructureKHR& TLAS,
                      vk::raii::Buffer& ubo,
                      vk::raii::ImageView& storageImageView);

            void updateDescriptorSets(vk::raii::ImageView& storageImageView);

            // gettery
            vk::raii::DescriptorPool & getDescriptorPool() { return descriptorPool; }
            vk::raii::DescriptorSet & getDescriptorSet() { return descriptorSet; }
            vk::raii::DescriptorSetLayout & getDescriptorSetLayout() { return descriptorSetLayout; }

            //settery
            void setDescriptorSetLayout(vk::raii::DescriptorSetLayout& layout) { descriptorSetLayout = std::move(layout); }

        private:
            void createDescriptorSets(vk::raii::AccelerationStructureKHR& TLAS,
                                vk::raii::Buffer& ubo,
                                vk::raii::ImageView& storageImageView);

        private:
            VULKAN_CONTEXT& ctx;

            vk::raii::DescriptorPool descriptorPool{nullptr};
            vk::raii::DescriptorSet descriptorSet{nullptr};
            vk::raii::DescriptorSetLayout descriptorSetLayout{nullptr};
    };
}
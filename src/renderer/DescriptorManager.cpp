#include "pch.h"
#include "DescriptorManager.hpp"

namespace VRTR
{
    DescriptorManager::DescriptorManager(VULKAN_CONTEXT& ctx)
        : ctx(ctx)
    {
    }

    void DescriptorManager::init(const DescriptorResources& resources)
    {
        descriptorResources = resources;

        assert(resources.TLAS);
        assert(resources.ubo);
        assert(resources.storageImageView);

        createDescriptorSets();
        updateDescriptorSets();
    }

    void DescriptorManager::createDescriptorSets()
    {
        VRTR_DEBUG("Creating Descriptor Sets");
        uint32_t maxSets = 1; // one for now, but later we will need more
        std::vector<vk::DescriptorPoolSize> poolSizes =
            {
                {vk::DescriptorType::eAccelerationStructureKHR, maxSets}, // wsparcie dla AS
                {vk::DescriptorType::eStorageImage, maxSets},             // umozliwienie zapisywania wyniku shaderow do storage image
                {vk::DescriptorType::eUniformBuffer, maxSets}             // wsparcie dla uniform bufferow (info ze sceny. np. macierz mvp)
            };

        // Descriptor Pool - zarządzanie pamiecią dla descriptor setów
        vk::DescriptorPoolCreateInfo poolInfo{
            .flags = {vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet},
            .maxSets = maxSets,
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes = poolSizes.data()};

        descriptorPool = vk::raii::DescriptorPool(ctx.logicalDevice, poolInfo);

        // Descriptor set - opis zasobów używanych przez shadery,
        // czyli layouty, bindingi ktore potem sie wykorzystujew shaderach
        vk::DescriptorSetAllocateInfo allocInfo{
            .descriptorPool = descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &*descriptorSetLayout};

        // A little workaround here, because Its not possible to create a single descriptor set in hpp vulkan
        // So I create descriptorSets (NOTE S on the end) and then move the first one to descriptorSet
        // https://github.com/KhronosGroup/Vulkan-Hpp/blob/938a2c36d2d3886a293c63c9a26417d6b0e2bc2d/vk_raii_ProgrammingGuide.md#09-create-a-vkraiidescriptorpool-and-vkraiidescriptorsets

        vk::raii::DescriptorSets tempDescriptorSets = vk::raii::DescriptorSets(ctx.logicalDevice, allocInfo);
        descriptorSet = std::move(tempDescriptorSets.front());

        vk::WriteDescriptorSetAccelerationStructureKHR descriptorASInfo{
            .pNext = nullptr,
            .accelerationStructureCount = 1,
            .pAccelerationStructures = &**descriptorResources.TLAS};

        vk::WriteDescriptorSet ASWrite{
            .pNext = &descriptorASInfo,
            .dstSet = *descriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eAccelerationStructureKHR,
        };

        vk::DescriptorImageInfo imageInfo{
            .sampler = {},
            .imageView = *descriptorResources.storageImageView,
            .imageLayout = vk::ImageLayout::eGeneral};

        vk::WriteDescriptorSet resultImageWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageImage,
            .pImageInfo = &imageInfo};

        vk::DescriptorBufferInfo bufferInfo{
            .buffer = *descriptorResources.ubo,
            .offset = 0,
            .range = vk::WholeSize};

        vk::WriteDescriptorSet uniformBufferWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &bufferInfo};

        std::array<vk::WriteDescriptorSet, 3> WriteDescriptorSets = {
            ASWrite,
            resultImageWrite,
            uniformBufferWrite};
        ctx.logicalDevice.updateDescriptorSets(WriteDescriptorSets, {});
    }

    void DescriptorManager::updateDescriptorSets()
    {
        vk::DescriptorImageInfo imageInfo{
            .sampler = {},
            .imageView = *descriptorResources.storageImageView,
            .imageLayout = vk::ImageLayout::eGeneral};

        vk::WriteDescriptorSet resultImageWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageImage,
            .pImageInfo = &imageInfo};
        std::array<vk::WriteDescriptorSet, 1> WriteDescriptorSets{resultImageWrite};
        ctx.logicalDevice.updateDescriptorSets(WriteDescriptorSets, {});
    }
}
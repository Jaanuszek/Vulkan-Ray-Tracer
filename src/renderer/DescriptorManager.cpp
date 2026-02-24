#include "pch.h"
#include "DescriptorManager.hpp"

namespace VRTR
{
    DescriptorManager::DescriptorManager(RendererContext& ctx)
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
        // TODO rozbić tą funkcje na mniejsze kawałki
        // Zrobic oddzielną funkcje do tworzenia descriptorSetLayout, descriptorPool i descriptorSet
        // Aktualnie ta funkcja robi za dużo
        VRTR_DEBUG("Creating Descriptor Sets");

        vk::DescriptorSetLayoutBinding ASLayout{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eAccelerationStructureKHR,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR,
            .pImmutableSamplers = nullptr};

        vk::DescriptorSetLayoutBinding storageImageLayout{
            .binding = 1,
            .descriptorType = vk::DescriptorType::eStorageImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR,
            .pImmutableSamplers = nullptr};

        vk::DescriptorSetLayoutBinding uniformBufferLayout{
            .binding = 2,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR,
            .pImmutableSamplers = nullptr};

        vk::DescriptorSetLayoutBinding textureBinding{
            .binding = 3,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR,
            .pImmutableSamplers = nullptr};

        vk::DescriptorSetLayoutBinding geometryInfoBufferLayout{
            .binding = 4,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR,
            .pImmutableSamplers = nullptr};

        vk::DescriptorSetLayoutBinding materialBufferLayout{
            .binding = 5,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR,
            .pImmutableSamplers = nullptr};

        std::array<vk::DescriptorSetLayoutBinding, 6> bindings =
            {
                ASLayout,
                storageImageLayout,
                uniformBufferLayout,
                textureBinding,
                geometryInfoBufferLayout,
                materialBufferLayout
            };

        vk::DescriptorSetLayoutCreateInfo layoutInfo{
            .flags = {},
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()};
        descriptorSetLayout = vk::raii::DescriptorSetLayout(ctx.logicalDevice, layoutInfo);
        
        uint32_t maxSets = 1; // one for now, but later we will need more
        std::vector<vk::DescriptorPoolSize> poolSizes =
            {
                {vk::DescriptorType::eAccelerationStructureKHR, maxSets}, // wsparcie dla AS
                {vk::DescriptorType::eStorageImage, maxSets},             // umozliwienie zapisywania wyniku shaderow do storage image
                {vk::DescriptorType::eUniformBuffer, maxSets},            // wsparcie dla uniform bufferow (info ze sceny. np. macierz mvp)
                {vk::DescriptorType::eCombinedImageSampler, maxSets},     // wsparcie dla tekstur
                {vk::DescriptorType::eStorageBuffer, maxSets},            // wsparcie dla geometry info buffera
                {vk::DescriptorType::eStorageBuffer, maxSets},            // wsparcie dla material buffera
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
            .pAccelerationStructures = &descriptorResources.TLAS};

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
            .imageView = descriptorResources.storageImageView,
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
            .buffer = descriptorResources.ubo,
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

        vk::DescriptorImageInfo textureImageInfo{
            .sampler = descriptorResources.texSampler,
            .imageView = descriptorResources.texImageView,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

        vk::WriteDescriptorSet textureWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 3,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &textureImageInfo};

        vk::DescriptorBufferInfo geometryInfoBufferInfo{
            .buffer = descriptorResources.geometryInfoBuffer,
            .offset = 0,
            .range = vk::WholeSize};

        vk::WriteDescriptorSet geometryInfoBufferWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 4,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &geometryInfoBufferInfo};

        vk::DescriptorBufferInfo materialBufferInfo{
            .buffer = descriptorResources.materialBuffer,
            .offset = 0,
            .range = vk::WholeSize};

        vk::WriteDescriptorSet materialBufferWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 5,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &materialBufferInfo};

        std::array<vk::WriteDescriptorSet, 6> WriteDescriptorSets = {
            ASWrite,
            resultImageWrite,
            uniformBufferWrite,
            textureWrite,
            geometryInfoBufferWrite,
            materialBufferWrite
        };
        ctx.logicalDevice.updateDescriptorSets(WriteDescriptorSets, {});
    }

    void DescriptorManager::updateDescriptorSets()
    {
        vk::DescriptorImageInfo imageInfo{
            .sampler = {},
            .imageView = descriptorResources.storageImageView,
            .imageLayout = vk::ImageLayout::eGeneral};

        vk::WriteDescriptorSet resultImageWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageImage,
            .pImageInfo = &imageInfo};

        vk::DescriptorImageInfo textureImageInfo{
            .sampler = descriptorResources.texSampler,
            .imageView = descriptorResources.texImageView,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

        vk::WriteDescriptorSet textureWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 3,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &textureImageInfo};

        vk::DescriptorBufferInfo geometryInfoBufferInfo{
            .buffer = descriptorResources.geometryInfoBuffer,
            .offset = 0,
            .range = vk::WholeSize};

        vk::WriteDescriptorSet geometryInfoBufferWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 4,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &geometryInfoBufferInfo};

        vk::DescriptorBufferInfo materialBufferInfo{
            .buffer = descriptorResources.materialBuffer,
            .offset = 0,
            .range = vk::WholeSize};

        vk::WriteDescriptorSet materialBufferWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 5,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &materialBufferInfo};

        std::array<vk::WriteDescriptorSet, 4> WriteDescriptorSets = {
            resultImageWrite,
            textureWrite,
            geometryInfoBufferWrite,
            materialBufferWrite
        };
        ctx.logicalDevice.updateDescriptorSets(WriteDescriptorSets, {});
    }
}
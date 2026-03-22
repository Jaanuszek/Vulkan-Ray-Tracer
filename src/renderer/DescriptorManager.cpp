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

    void DescriptorManager::createDescriptorSetLayout()
    {
        VRTR_DEBUG("Creating Descriptor Set Layout");
        vk::DescriptorSetLayoutBinding ASLayout{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eAccelerationStructureKHR,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR,
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
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR,
            .pImmutableSamplers = nullptr};

        vk::DescriptorSetLayoutBinding textureBinding{
            .binding = 3,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            // .descriptorCount = CONSTANTS::MAX_TEXTURES,
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

        vk::DescriptorSetLayoutBinding cudaColorBufferLayout{
            .binding = 6,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR,
            .pImmutableSamplers = nullptr};

        vk::DescriptorSetLayoutBinding triToPatchBufferLayout{
            .binding = 7,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR,
            .pImmutableSamplers = nullptr};

        vk::DescriptorSetLayoutBinding patchBufferLayout{
            .binding = 8,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR,
            .pImmutableSamplers = nullptr};

        vk::DescriptorSetLayoutBinding selectedPatchLayout{
            .binding = 9,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR,
            .pImmutableSamplers = nullptr};

        vk::DescriptorSetLayoutBinding radiosityLightmapLayout{
            .binding = 10,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR,
            .pImmutableSamplers = nullptr};

        std::vector<vk::DescriptorSetLayoutBinding> bindings = {
            ASLayout,
            storageImageLayout,
            uniformBufferLayout,
            textureBinding,
            geometryInfoBufferLayout,
            materialBufferLayout,
            cudaColorBufferLayout,
            triToPatchBufferLayout,
            patchBufferLayout,
            selectedPatchLayout,
            radiosityLightmapLayout
        };

        vk::DescriptorSetLayoutCreateInfo layoutInfo{
            .flags = {},
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()};
        descriptorSetLayout = vk::raii::DescriptorSetLayout(ctx.logicalDevice, layoutInfo);
    }

    void DescriptorManager::createDescriptorPool()
    {
        VRTR_DEBUG("Creating Descriptor Pool");
        constexpr uint32_t maxSets = 1; // one for now, but later we will need more
        std::vector<vk::DescriptorPoolSize> poolSizes =
            {
                {vk::DescriptorType::eAccelerationStructureKHR, maxSets},              // wsparcie dla AS
                {vk::DescriptorType::eStorageImage, maxSets},                          // umozliwienie zapisywania wyniku shaderow do storage image
                {vk::DescriptorType::eUniformBuffer, maxSets},                         // wsparcie dla uniform bufferow (info ze sceny. np. macierz mvp)
                // {vk::DescriptorType::eCombinedImageSampler, CONSTANTS::MAX_TEXTURES},  // wsparcie dla tekstur
                {vk::DescriptorType::eCombinedImageSampler, 1}, 
                {vk::DescriptorType::eStorageBuffer, maxSets},            // wsparcie dla geometry info buffera
                {vk::DescriptorType::eStorageBuffer, maxSets},            // wsparcie dla material buffera
                {vk::DescriptorType::eStorageBuffer, maxSets},            // wsparcie dla CUDA color buffera
                {vk::DescriptorType::eStorageBuffer, maxSets},            // wsparcie dla triToPatch buffera    
                {vk::DescriptorType::eStorageBuffer, maxSets},             // wsparcie dla patch buffera
                {vk::DescriptorType::eStorageBuffer, maxSets},             // wsparcie dla selected patch buffera
                {vk::DescriptorType::eStorageBuffer, maxSets},             // wsparcie dla radiosity lightmap buffera
            };

        // Descriptor Pool - zarządzanie pamiecią dla descriptor setów
        vk::DescriptorPoolCreateInfo poolInfo{
            .flags = {vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet},
            .maxSets = maxSets,
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes = poolSizes.data()};

        descriptorPool = vk::raii::DescriptorPool(ctx.logicalDevice, poolInfo);
    }

    void DescriptorManager::allocateDescriptorSet()
    {
        vk::DescriptorSetAllocateInfo allocInfo{
            .descriptorPool = descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &*descriptorSetLayout};
        // https://github.com/KhronosGroup/Vulkan-Hpp/blob/938a2c36d2d3886a293c63c9a26417d6b0e2bc2d/vk_raii_ProgrammingGuide.md#09-create-a-vkraiidescriptorpool-and-vkraiidescriptorsets

        vk::raii::DescriptorSets tempDescriptorSets = vk::raii::DescriptorSets(ctx.logicalDevice, allocInfo);
        descriptorSet = std::move(tempDescriptorSets.front());
    }

    void DescriptorManager::writeDescriptorSet()
    {
        VRTR_DEBUG("Writing Descriptor Sets");
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

        // TODO zajac sie dodaniem wiekszej ilosci tekstud do shaderow
        // assert(descriptorResources.texImageViews.size() == CONSTANTS::MAX_TEXTURES);
        // assert(descriptorResources.texSamplers.size() == CONSTANTS::MAX_TEXTURES);

        // std::vector<vk::DescriptorImageInfo> textureImageInfos;
        // textureImageInfos.reserve(CONSTANTS::MAX_TEXTURES);
        // for (size_t i = 0; i < CONSTANTS::MAX_TEXTURES; ++i)
        // {
        //     textureImageInfos.emplace_back(vk::DescriptorImageInfo{
        //         .sampler = descriptorResources.texSamplers[i],
        //         .imageView = descriptorResources.texImageViews[i],
        //         .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        //     });
        // }
        // TEMPORARY tylko jedna pierwsza tekstura idzie do shadera
        std::vector<vk::DescriptorImageInfo> textureImageInfos;
        textureImageInfos.push_back(vk::DescriptorImageInfo{
            .sampler = descriptorResources.texSamplers[0],
            .imageView = descriptorResources.texImageViews[0],
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        });

        vk::WriteDescriptorSet textureWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 3,
            .dstArrayElement = 0,
            .descriptorCount = static_cast<uint32_t>(textureImageInfos.size()),
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = textureImageInfos.data()};

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

        vk::DescriptorBufferInfo cudaColorBufferInfo{
            .buffer = descriptorResources.cudaColorBuffer,
            .offset = 0,
            .range = vk::WholeSize};

        vk::WriteDescriptorSet cudaColorBufferWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 6,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &cudaColorBufferInfo};

        vk::DescriptorBufferInfo triToPatchBufferInfo{
            .buffer = descriptorResources.triToPatchBuffer,
            .offset = 0,
            .range = vk::WholeSize};

        vk::WriteDescriptorSet triToPatchBufferWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 7,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &triToPatchBufferInfo};

        vk::DescriptorBufferInfo patchBufferInfo{
            .buffer = descriptorResources.patchBuffer,
            .offset = 0,
            .range = vk::WholeSize};

        vk::WriteDescriptorSet patchBufferWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 8,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &patchBufferInfo};

        vk::DescriptorBufferInfo selectedPatchBufferInfo{
            .buffer = descriptorResources.selectedPatchBuffer,
            .offset = 0,
            .range = vk::WholeSize};

        vk::WriteDescriptorSet selectedPatchBufferWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 9,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &selectedPatchBufferInfo};

        vk::DescriptorBufferInfo radiosityLightmapBufferInfo{
            .buffer = descriptorResources.radiosityLightmapBuffer,
            .offset = 0,
            .range = vk::WholeSize};

        vk::WriteDescriptorSet radiosityLightmapBufferWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 10,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &radiosityLightmapBufferInfo};

        std::vector<vk::WriteDescriptorSet> WriteDescriptorSets = {
            ASWrite,
            resultImageWrite,
            uniformBufferWrite,
            textureWrite,
            geometryInfoBufferWrite,
            materialBufferWrite,
            cudaColorBufferWrite,
            triToPatchBufferWrite,
            patchBufferWrite,
            selectedPatchBufferWrite,
            radiosityLightmapBufferWrite
        };
        ctx.logicalDevice.updateDescriptorSets(WriteDescriptorSets, {});
    }

    void DescriptorManager::createDescriptorSets()
    {
        VRTR_DEBUG("Creating Descriptor Sets");

        createDescriptorSetLayout();

        createDescriptorPool();

        allocateDescriptorSet();

        writeDescriptorSet();
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

        // texImageViews must already be padded to MAX_TEXTURES (done in Scene::updateDescriptorResources)
        // assert(descriptorResources.texImageViews.size() == CONSTANTS::MAX_TEXTURES);
        // assert(descriptorResources.texSamplers.size() == CONSTANTS::MAX_TEXTURES);

        // std::vector<vk::DescriptorImageInfo> textureImageInfos;
        // textureImageInfos.reserve(CONSTANTS::MAX_TEXTURES);
        // for (size_t i = 0; i < CONSTANTS::MAX_TEXTURES; ++i)
        // {
        //     textureImageInfos.emplace_back(vk::DescriptorImageInfo{
        //         .sampler = descriptorResources.texSamplers[i],
        //         .imageView = descriptorResources.texImageViews[i],
        //         .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        //     });
        // }

        std::vector<vk::DescriptorImageInfo> textureImageInfos;
        textureImageInfos.push_back(vk::DescriptorImageInfo{
            .sampler = descriptorResources.texSamplers[0],
            .imageView = descriptorResources.texImageViews[0],
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        });

        vk::WriteDescriptorSet textureWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 3,
            .dstArrayElement = 0,
            .descriptorCount = static_cast<uint32_t>(textureImageInfos.size()),
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = textureImageInfos.data()};

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

        vk::DescriptorBufferInfo cudaColorBufferInfo{
            .buffer = descriptorResources.cudaColorBuffer,
            .offset = 0,
            .range = vk::WholeSize};

        vk::WriteDescriptorSet cudaColorBufferWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 6,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &cudaColorBufferInfo};

        vk::DescriptorBufferInfo triToPatchBufferInfo{
            .buffer = descriptorResources.triToPatchBuffer,
            .offset = 0,
            .range = vk::WholeSize};

        vk::WriteDescriptorSet triToPatchBufferWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 7,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &triToPatchBufferInfo};

        vk::DescriptorBufferInfo patchBufferInfo{
            .buffer = descriptorResources.patchBuffer,
            .offset = 0,
            .range = vk::WholeSize};

        vk::WriteDescriptorSet patchBufferWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 8,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &patchBufferInfo};

        vk::DescriptorBufferInfo selectedPatchBufferInfo{
            .buffer = descriptorResources.selectedPatchBuffer,
            .offset = 0,
            .range = vk::WholeSize};

        vk::WriteDescriptorSet selectedPatchBufferWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 9,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &selectedPatchBufferInfo};

        vk::DescriptorBufferInfo radiosityLightmapBufferInfo{
            .buffer = descriptorResources.radiosityLightmapBuffer,
            .offset = 0,
            .range = vk::WholeSize};

        vk::WriteDescriptorSet radiosityLightmapBufferWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 10,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &radiosityLightmapBufferInfo};

        std::vector<vk::WriteDescriptorSet> WriteDescriptorSets = {
            resultImageWrite,
            textureWrite,
            geometryInfoBufferWrite,
            materialBufferWrite,
            cudaColorBufferWrite,
            triToPatchBufferWrite,
            patchBufferWrite,
            selectedPatchBufferWrite,
            radiosityLightmapBufferWrite
        };
        ctx.logicalDevice.updateDescriptorSets(WriteDescriptorSets, {});
    }
}
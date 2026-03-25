#include "pch.h"
#include "VisibilityDescriptorManager.hpp"

namespace VRTR
{
    VisibilityDescriptorManager::VisibilityDescriptorManager(RendererContext& ctx)
        : ctx(ctx)
    {
    }

    void VisibilityDescriptorManager::init(const DescriptorResources& resources)
    {
        descriptorResources = resources;

        assert(resources.TLAS);
        assert(resources.ubo);

        createDescriptorSets();
        updateDescriptorSets();
    }

    void VisibilityDescriptorManager::createDescriptorSets()
    {
        VRTR_DEBUG("Creating Descriptor Sets");

        createDescriptorSetLayout();

        createDescriptorPool();

        allocateDescriptorSet();

        writeDescriptorSet();
    }

    void VisibilityDescriptorManager::createDescriptorSetLayout()
    {
        VRTR_DEBUG("Creating Descriptor Set Layout");
        vk::DescriptorSetLayoutBinding ASLayout{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eAccelerationStructureKHR,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR,
            .pImmutableSamplers = nullptr};

        vk::DescriptorSetLayoutBinding uniformBufferLayout{
            .binding = 2,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR,
            .pImmutableSamplers = nullptr};

        vk::DescriptorSetLayoutBinding geometryInfoBufferLayout{
            .binding = 4,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR,
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
            .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eAnyHitKHR,
            .pImmutableSamplers = nullptr};

        vk::DescriptorSetLayoutBinding patchBufferLayout{
            .binding = 8,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eRaygenKHR,
            .pImmutableSamplers = nullptr};

        vk::DescriptorSetLayoutBinding selectedPatchLayout{
            .binding = 9,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eRaygenKHR,
            .pImmutableSamplers = nullptr};

        vk::DescriptorSetLayoutBinding patchVisibilityLayout{
            .binding = 10,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eRaygenKHR,
            .pImmutableSamplers = nullptr};

        std::vector<vk::DescriptorSetLayoutBinding> bindings = {
            ASLayout,
            uniformBufferLayout,
            geometryInfoBufferLayout,
            materialBufferLayout,
            cudaColorBufferLayout,
            triToPatchBufferLayout,
            patchBufferLayout,
            selectedPatchLayout,
            patchVisibilityLayout
        };

        vk::DescriptorSetLayoutCreateInfo layoutInfo{
            .flags = {},
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()};
        descriptorSetLayout = vk::raii::DescriptorSetLayout(ctx.logicalDevice, layoutInfo);
    }

    void VisibilityDescriptorManager::createDescriptorPool()
    {
        VRTR_DEBUG("Creating Descriptor Pool");
        constexpr uint32_t maxSets = 1; // one for now, but later we will need more
        std::vector<vk::DescriptorPoolSize> poolSizes =
            {
                {vk::DescriptorType::eAccelerationStructureKHR, maxSets},              // wsparcie dla AS
                {vk::DescriptorType::eUniformBuffer, maxSets},                         // wsparcie dla uniform bufferow (info ze sceny. np. macierz mvp)
                {vk::DescriptorType::eStorageBuffer, maxSets},            // wsparcie dla geometry info buffera
                {vk::DescriptorType::eStorageBuffer, maxSets},            // wsparcie dla material buffera
                {vk::DescriptorType::eStorageBuffer, maxSets},            // wsparcie dla CUDA color buffera
                {vk::DescriptorType::eStorageBuffer, maxSets},            // wsparcie dla triToPatch buffera    
                {vk::DescriptorType::eStorageBuffer, maxSets},             // wsparcie dla patch buffera
                {vk::DescriptorType::eStorageBuffer, maxSets},             // wsparcie dla selected patch buffera
                {vk::DescriptorType::eStorageBuffer, maxSets},             // wsparcie dla patch visibility buffera
            };

        // Descriptor Pool - zarządzanie pamiecią dla descriptor setów
        vk::DescriptorPoolCreateInfo poolInfo{
            .flags = {vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet},
            .maxSets = maxSets,
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes = poolSizes.data()};

        descriptorPool = vk::raii::DescriptorPool(ctx.logicalDevice, poolInfo);
    }

    void VisibilityDescriptorManager::allocateDescriptorSet()
    {
        vk::DescriptorSetAllocateInfo allocInfo{
            .descriptorPool = descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &*descriptorSetLayout};
        // https://github.com/KhronosGroup/Vulkan-Hpp/blob/938a2c36d2d3886a293c63c9a26417d6b0e2bc2d/vk_raii_ProgrammingGuide.md#09-create-a-vkraiidescriptorpool-and-vkraiidescriptorsets

        vk::raii::DescriptorSets tempDescriptorSets = vk::raii::DescriptorSets(ctx.logicalDevice, allocInfo);
        descriptorSet = std::move(tempDescriptorSets.front());
    }

    void VisibilityDescriptorManager::writeDescriptorSet()
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

        vk::DescriptorBufferInfo patchVisibilityBufferInfo{
            .buffer = descriptorResources.patchVisibilityBuffer,
            .offset = 0,
            .range = vk::WholeSize};

        vk::WriteDescriptorSet patchVisibilityBufferWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 10,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &patchVisibilityBufferInfo};

        std::vector<vk::WriteDescriptorSet> WriteDescriptorSets = {
            ASWrite,
            uniformBufferWrite,
            geometryInfoBufferWrite,
            materialBufferWrite,
            cudaColorBufferWrite,
            triToPatchBufferWrite,
            patchBufferWrite,
            selectedPatchBufferWrite,
            patchVisibilityBufferWrite
        };
        ctx.logicalDevice.updateDescriptorSets(WriteDescriptorSets, {});
    }

    void VisibilityDescriptorManager::updateDescriptorSets()
    {
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

        vk::DescriptorBufferInfo patchVisibilityBufferInfo{
            .buffer = descriptorResources.patchVisibilityBuffer,
            .offset = 0,
            .range = vk::WholeSize};

        vk::WriteDescriptorSet patchVisibilityBufferWrite{
            .pNext = nullptr,
            .dstSet = *descriptorSet,
            .dstBinding = 10,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &patchVisibilityBufferInfo};

        std::vector<vk::WriteDescriptorSet> WriteDescriptorSets = {
            geometryInfoBufferWrite,
            materialBufferWrite,
            cudaColorBufferWrite,
            triToPatchBufferWrite,
            patchBufferWrite,
            selectedPatchBufferWrite,
            patchVisibilityBufferWrite
        };
        ctx.logicalDevice.updateDescriptorSets(WriteDescriptorSets, {});
    }
}
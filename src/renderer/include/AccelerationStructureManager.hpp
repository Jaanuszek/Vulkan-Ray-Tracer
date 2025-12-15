#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"
#include "AccelerationStructureUtils.hpp"

namespace VRTR
{
    class AccelerationStructureManager
    {
        public:
            AccelerationStructureManager();

            uint32_t createBLAS(const std::vector<VertexRT>& vertices,
                                const std::vector<uint32_t>& indices);

            void buildTLAS();

            void addInstance(uint32_t blasIdx, const glm::mat4 &transform);

            void updateTLAS();

            vk::raii::AccelerationStructureKHR& getTLAS() { return tlas.as.handle; }

        private:
            VULKAN_CONTEXT ctx{};
            // TODO change to RendererContext

            // Instances data  blasID, transofmr, mask, customidx, hitGroupIndex
            std::vector<AS::InstanceData> instances;

            std::vector<AS::BottomLevelAS> blasList;
            AS::TopLevelAS tlas;
    };
}
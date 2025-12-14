#pragma once

#include "Logger.hpp"
#include "ConstantsAndStructs.hpp"
#include "AccelerationStructureUtils.hpp"

namespace VRTR
{
    struct AccelerationStructure
    {
        vk::raii::AccelerationStructureKHR handle{nullptr};
        vk::DeviceAddress deviceAddress{0};
        std::unique_ptr<Buffer> buffer{nullptr};
    };

    struct InstanceData
    {
        glm::mat4 transform;
        uint32_t blasIdx;
        uint32_t customIdx;
        uint32_t mask;
        uint32_t hitGroupIndex; // used to fetch the shaders from the SBT
    };

    class AccelerationStructureManager
    {
        public:
            AccelerationStructureManager();

            uint32_t createBLAS(const std::vector<VertexRT>& vertices,
                                const std::vector<uint32_t>& indices);

            void buildTLAS();

            void addInstance(uint32_t blasIdx, const glm::mat4 &transform);

            void updateTLAS();

            vk::raii::AccelerationStructureKHR& getTLAS() { return tlas.handle; }

        private:
            VULKAN_CONTEXT ctx{};
            // TODO change to RendererContext

            std::vector<AccelerationStructure> blasList;
            AccelerationStructure tlas;
            std::vector<InstanceData> instances;
    };
}
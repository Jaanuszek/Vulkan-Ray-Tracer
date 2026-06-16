#pragma once
#include <glm/glm.hpp>

#include "CommonStructs.h"

namespace VRTR
{
    namespace CustomModels
    {
        void appendGridPlane(std::vector<VertexRT> &vertices,
                             std::vector<uint32_t> &indices,
                             const glm::vec3 &center,
                             const glm::vec3 &uAxis,
                             const glm::vec3 &vAxis,
                             const glm::vec3 &normal,
                             uint32_t uSegments,
                             uint32_t vSegments,
                             float uSize,
                             float vSize,
                             const glm::vec3 &color);

        std::pair<std::vector<VertexRT>, std::vector<uint32_t>> createDividedRectangle(
            uint32_t xSegments,
            uint32_t zSegments,
            float width,
            float depth,
            const glm::vec3 &color);

        std::pair<std::vector<VertexRT>, std::vector<uint32_t>> createDividedCube(
            uint32_t xSegments,
            uint32_t ySegments,
            uint32_t zSegments,
            float size,
            const glm::vec3 &color);

        std::pair<std::vector<VertexRT>, std::vector<uint32_t>> createSphere(uint32_t stackCount = 24, uint32_t sectorCount = 48, float radius = 1.0f, const glm::vec3& color = glm::vec3(0.0f));
    }
}
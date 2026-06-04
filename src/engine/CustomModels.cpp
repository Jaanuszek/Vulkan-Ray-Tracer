#include "pch.h"
#include "CustomModels.hpp"

namespace VRTR
{
    namespace CustomModels
    {
        void appendGridPlane(std::vector<VertexRT>& vertices,
                            std::vector<uint32_t>& indices,
                            const glm::vec3& center,
                            const glm::vec3& uAxis,
                            const glm::vec3& vAxis,
                            const glm::vec3& normal,
                            uint32_t uSegments,
                            uint32_t vSegments,
                            float uSize,
                            float vSize,
                            const glm::vec3& color)
        {
            uSegments = std::max<uint32_t>(1, uSegments);
            vSegments = std::max<uint32_t>(1, vSegments);

            const uint32_t baseVertex = static_cast<uint32_t>(vertices.size());

            for (uint32_t v = 0; v <= vSegments; ++v)
            {
                const float fv = static_cast<float>(v) / static_cast<float>(vSegments);
                const float vOffset = (fv - 0.5f) * vSize;

                for (uint32_t u = 0; u <= uSegments; ++u)
                {
                    const float fu = static_cast<float>(u) / static_cast<float>(uSegments);
                    const float uOffset = (fu - 0.5f) * uSize;
                    const glm::vec3 position = center + uAxis * uOffset + vAxis * vOffset;

                    vertices.push_back(VertexRT{
                        position,
                        normal,
                        color,
                        {fu, fv}
                    });
                }
            }

            const uint32_t rowStride = uSegments + 1;
            for (uint32_t v = 0; v < vSegments; ++v)
            {
                for (uint32_t u = 0; u < uSegments; ++u)
                {
                    const uint32_t i0 = baseVertex + v * rowStride + u;
                    const uint32_t i1 = i0 + 1;
                    const uint32_t i2 = i0 + rowStride;
                    const uint32_t i3 = i2 + 1;

                    indices.push_back(i0);
                    indices.push_back(i2);
                    indices.push_back(i1);

                    indices.push_back(i1);
                    indices.push_back(i2);
                    indices.push_back(i3);
                }
            }
        }

        std::pair<std::vector<VertexRT>, std::vector<uint32_t>> createDividedRectangle(
            uint32_t xSegments,
            uint32_t zSegments,
            float width,
            float depth,
            const glm::vec3& color)
        {
            std::vector<VertexRT> vertices;
            std::vector<uint32_t> indices;
            appendGridPlane(
                vertices,
                indices,
                glm::vec3(0.0f),
                glm::vec3(1.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 0.0f, 1.0f),
                glm::vec3(0.0f, 1.0f, 0.0f),
                xSegments,
                zSegments,
                width,
                depth,
                color);

            return {vertices, indices};
        }

        std::pair<std::vector<VertexRT>, std::vector<uint32_t>> createDividedCube(
            uint32_t xSegments,
            uint32_t ySegments,
            uint32_t zSegments,
            float size,
            const glm::vec3& color)
        {
            xSegments = std::max<uint32_t>(1, xSegments);
            ySegments = std::max<uint32_t>(1, ySegments);
            zSegments = std::max<uint32_t>(1, zSegments);

            std::vector<VertexRT> vertices;
            std::vector<uint32_t> indices;
            const float half = size * 0.5f;

            appendGridPlane(vertices, indices, {0.0f, 0.0f, half},  {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},   xSegments, ySegments, size, size, color);
            appendGridPlane(vertices, indices, {0.0f, 0.0f, -half}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f},  xSegments, ySegments, size, size, color);
            appendGridPlane(vertices, indices, {half, 0.0f, 0.0f},   {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f},   zSegments, ySegments, size, size, color);
            appendGridPlane(vertices, indices, {-half, 0.0f, 0.0f},  {0.0f, 0.0f, 1.0f},  {0.0f, 1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},  zSegments, ySegments, size, size, color);
            appendGridPlane(vertices, indices, {0.0f, half, 0.0f},   {1.0f, 0.0f, 0.0f},  {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, xSegments, zSegments, size, size, color);
            appendGridPlane(vertices, indices, {0.0f, -half, 0.0f},  {1.0f, 0.0f, 0.0f},  {0.0f, 0.0f, 1.0f},  {0.0f, -1.0f, 0.0f}, xSegments, zSegments, size, size, color);

            return {vertices, indices};
        }

        std::pair<std::vector<VertexRT>, std::vector<uint32_t>> createSphere(uint32_t stackCount, uint32_t sectorCount, float radius, const glm::vec3& color)
        {
            stackCount = std::max<uint32_t>(3, stackCount);
            sectorCount = std::max<uint32_t>(3, sectorCount);

            std::vector<VertexRT> vertices;
            std::vector<uint32_t> indices;
            vertices.reserve((stackCount + 1) * (sectorCount + 1));
            indices.reserve(stackCount * sectorCount * 6);

            for (uint32_t stack = 0; stack <= stackCount; ++stack)
            {
                const float stackAngle = glm::pi<float>() * 0.5f - static_cast<float>(stack) * glm::pi<float>() / static_cast<float>(stackCount);
                const float xy = radius * std::cos(stackAngle);
                const float z = radius * std::sin(stackAngle);

                for (uint32_t sector = 0; sector <= sectorCount; ++sector)
                {
                    const float sectorAngle = static_cast<float>(sector) * 2.0f * glm::pi<float>() / static_cast<float>(sectorCount);
                    const float x = xy * std::cos(sectorAngle);
                    const float y = xy * std::sin(sectorAngle);

                    const glm::vec3 position{x, y, z};
                    const glm::vec3 normal = glm::normalize(position);

                    vertices.push_back(VertexRT{
                        position,
                        normal,
                        color,
                        {static_cast<float>(sector) / static_cast<float>(sectorCount), static_cast<float>(stack) / static_cast<float>(stackCount)}
                    });
                }
            }

            for (uint32_t stack = 0; stack < stackCount; ++stack)
            {
                const uint32_t k1 = stack * (sectorCount + 1);
                const uint32_t k2 = k1 + sectorCount + 1;

                for (uint32_t sector = 0; sector < sectorCount; ++sector)
                {
                    if (stack != 0)
                    {
                        indices.push_back(k1 + sector);
                        indices.push_back(k2 + sector);
                        indices.push_back(k1 + sector + 1);
                    }

                    if (stack != (stackCount - 1))
                    {
                        indices.push_back(k1 + sector + 1);
                        indices.push_back(k2 + sector);
                        indices.push_back(k2 + sector + 1);
                    }
                }
            }

            return {vertices, indices};
        }
    }
}
#include "Model.hpp"

// it has to be there!
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace
{
    constexpr float DEFAULT_MAX_WORLD_TRIANGLE_AREA = 0.0001f;
    constexpr uint32_t DEFAULT_MAX_SUBDIV_DEPTH = 128;
    constexpr float EPSILON = 1e-8f;

    VRTR::VertexRT midpointVertex(const VRTR::VertexRT& a, const VRTR::VertexRT& b)
    {
        VRTR::VertexRT v{};
        v.pos = (a.pos + b.pos) / 2.0f;

        const glm::vec3 n = (a.normal + b.normal) / 2.0f;
        if (glm::length(n) > EPSILON)
        {
            v.normal = glm::normalize(n);
        }
        else
        {
            v.normal = a.normal;
        }

        v.texCoord = (a.texCoord + b.texCoord) / 2.0f;
        return v;
    }

    void appendTriangle(const VRTR::VertexRT& v0,
                        const VRTR::VertexRT& v1,
                        const VRTR::VertexRT& v2,
                        std::vector<VRTR::VertexRT>& outVertices,
                        std::vector<uint32_t>& outIndices)
    {
        const uint32_t base = static_cast<uint32_t>(outVertices.size());
        outVertices.push_back(v0);
        outVertices.push_back(v1);
        outVertices.push_back(v2);

        outIndices.push_back(base + 0);
        outIndices.push_back(base + 1);
        outIndices.push_back(base + 2);
    }
}

namespace VRTR
{
    Model::Model(RendererContext& ctx,
                const std::string& modelPath, const std::string& texturePath,
                const Material& mat, const glm::mat4& transform)
        : ctx(ctx), material(mat), modelTransform(transform)
    {
        VRTR_DEBUG("Creating model from path: {}", modelPath);

        modelName = getModelNameFromPath(modelPath);

        if(!texturePath.empty())
            withTexture = true;

        loadModel(modelPath);
        tessellateLargeTriangles(DEFAULT_MAX_WORLD_TRIANGLE_AREA, DEFAULT_MAX_SUBDIV_DEPTH);
        createVertexBuffer();
        createIndexBuffer();
        setGeometryInfo();

        #ifdef enableRadiosity
            buildPatches(1);
            removeDuplicateVertices();
            buildVertexPatchAdjacency();
        #endif

        if (withTexture)
        {
            loadTexture(texturePath);
        }
    }

    Model::Model(RendererContext& ctx,
        const std::vector<VertexRT>& vertices, const std::vector<uint32_t>& indices,
        const Material& mat, const glm::mat4& transform)
        : ctx(ctx), material(mat), modelTransform(transform)
    {
        VRTR_DEBUG("Creating model from vertices and indices");

        modelMesh = std::make_unique<mesh>();
        modelMesh->vertices = vertices;
        modelMesh->indices = indices;
        material = mat;

        tessellateLargeTriangles(DEFAULT_MAX_WORLD_TRIANGLE_AREA, DEFAULT_MAX_SUBDIV_DEPTH);

        createVertexBuffer();
        createIndexBuffer();
        setGeometryInfo();

        #ifdef enableRadiosity
            buildPatches(1);
            removeDuplicateVertices();
            buildVertexPatchAdjacency();
        #endif
    }

    Model::~Model()
    {
        VRTR_DEBUG("Destroying model: {}", modelName);
    }

    void Model::loadModel(const std::string &path)
    {
        VRTR_DEBUG("Loading model from path: {}", path);

        assert(std::filesystem::exists(path));

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;
        bool containTexture = false;

        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str());
        if(!warn.empty())
        {
            VRTR_WARN("Model loading warning: {}", warn);
        }
        if(!err.empty())
        {
            VRTR_ERROR("Model loading error: {}", err);
        }
        if (!ret)
        {
            VRTR_ERROR("Failed to load model: {}", err);
            exit(1);
        }

        mesh Mesh{};

        for (const auto& shape : shapes)
        {
            for(const auto& idx : shape.mesh.indices)
            {
                VertexRT vertex{};

                vertex.pos = {
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2]
                };

                vertex.normal = {
                    attrib.normals[3 * idx.normal_index + 0],
                    attrib.normals[3 * idx.normal_index + 1],
                    attrib.normals[3 * idx.normal_index + 2]
                };

                if(!attrib.colors.empty())
                {
                    vertex.color = {
                        attrib.colors[3 * idx.vertex_index + 0],
                        attrib.colors[3 * idx.vertex_index + 1],
                        attrib.colors[3 * idx.vertex_index + 2]
                    };
                }
                else
                {
                    vertex.color = material.albedo;
                }

                if(withTexture) // bede tu mial puste texCoordy, co jest niewydajne. Miej o tym swiadomość
                {
                    vertex.texCoord = {
                        attrib.texcoords[2 * idx.texcoord_index + 0],
                        attrib.texcoords[2 * idx.texcoord_index + 1]
                    };
                }

                Mesh.vertices.push_back(vertex);
                Mesh.indices.push_back(Mesh.indices.size());
            }
        }
        modelMesh = std::make_unique<mesh>(std::move(Mesh));
    }

    void Model::tessellateLargeTriangles(float maxWorldTriangleArea, uint32_t maxDepth)
    {
        if (!modelMesh || modelMesh->indices.size() < 3)
        {
            return;
        }

        const size_t originalTriangleCount = modelMesh->indices.size() / 3;
        std::vector<VertexRT> refinedVertices;
        std::vector<uint32_t> refinedIndices;
        refinedVertices.reserve(modelMesh->vertices.size());
        refinedIndices.reserve(modelMesh->indices.size());

        auto subdivide = [&](auto&& self,
                             const VertexRT& v0,
                             const VertexRT& v1,
                             const VertexRT& v2,
                             uint32_t depth) -> void
        {
            const glm::vec3 p0 = glm::vec3(modelTransform * glm::vec4(v0.pos, 1.0f));
            const glm::vec3 p1 = glm::vec3(modelTransform * glm::vec4(v1.pos, 1.0f));
            const glm::vec3 p2 = glm::vec3(modelTransform * glm::vec4(v2.pos, 1.0f));

            const float area = 0.5f * glm::length(glm::cross(p1 - p0, p2 - p0));
            if (area <= maxWorldTriangleArea || depth >= maxDepth)
            {
                appendTriangle(v0, v1, v2, refinedVertices, refinedIndices);
                return;
            }

            // Uniform 1->4 split gives denser, more even tessellation near edges.
            const VertexRT m01 = midpointVertex(v0, v1);
            const VertexRT m12 = midpointVertex(v1, v2);
            const VertexRT m20 = midpointVertex(v2, v0);

            self(self, v0,  m01, m20, depth + 1);
            self(self, m01, v1,  m12, depth + 1);
            self(self, m20, m12, v2,  depth + 1);
            self(self, m01, m12, m20, depth + 1);
        };

        for (size_t tri = 0; tri < originalTriangleCount; ++tri)
        {
            const uint32_t i0 = modelMesh->indices[3 * tri + 0];
            const uint32_t i1 = modelMesh->indices[3 * tri + 1];
            const uint32_t i2 = modelMesh->indices[3 * tri + 2];

            const VertexRT& v0 = modelMesh->vertices[i0];
            const VertexRT& v1 = modelMesh->vertices[i1];
            const VertexRT& v2 = modelMesh->vertices[i2];
            subdivide(subdivide, v0, v1, v2, 0);
        }

        const size_t refinedTriangleCount = refinedIndices.size() / 3;
        if (refinedTriangleCount == originalTriangleCount)
        {
            return;
        }

        VRTR_INFO("Adaptive tessellation '{}': triangles {} -> {}", modelName, originalTriangleCount, refinedTriangleCount);

        modelMesh->vertices = std::move(refinedVertices);
        modelMesh->indices = std::move(refinedIndices);
    }

    const VmaAllocationCreateInfo Model::getVmaAllocCreateInfo()
    {
        // Te flagi sa w miare wolne, bo pozwalaja na odczyt cdanych z CPU
        // w przyszlosci fajnie by bylo miec dwa bufory, jeden ktory lezy na GPU
        // a drugi staging ktory pozwala na kopiowanie danych z CPU do GPU
        VmaAllocationCreateInfo allocInfo{
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };
        
        return allocInfo;
    }

    void Model::createVertexBuffer()
    {
        vk::DeviceSize bufferSize = modelMesh->vertices.size() * sizeof(VertexRT);
        vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        auto vmaAllocInfo = getVmaAllocCreateInfo();

        vertexBuffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.vmaAllocator, bufferSize, usage, vmaAllocInfo);
        vertexBuffer->Update(modelMesh->vertices.data(), bufferSize);
    }

    void Model::createIndexBuffer()
    {
        vk::DeviceSize bufferSize = modelMesh->indices.size() * sizeof(uint32_t);
        vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        auto vmaAllocInfo = getVmaAllocCreateInfo();

        indexBuffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.vmaAllocator, bufferSize, usage, vmaAllocInfo);
        indexBuffer->Update(modelMesh->indices.data(), bufferSize);
    }

    void Model::setGeometryInfo()
    {
        vk::BufferDeviceAddressInfo addrInfo{
            .buffer = vertexBuffer->getBufferHandle()
        };
        geometryInfo.vertexBufferAddr = ctx.logicalDevice.getBufferAddress(addrInfo);

        addrInfo.buffer = indexBuffer->getBufferHandle();
        geometryInfo.indexBufferAddr = ctx.logicalDevice.getBufferAddress(addrInfo);
    }

    void Model::loadTexture(const std::string &path)
    {
        VRTR_DEBUG("Loading texture from path: {}", path);

        assert(std::filesystem::exists(path));

        texture = std::make_unique<Texture>(ctx, path);
    }

    void Model::buildPatches(uint8_t patchSize)
    {
        const uint32_t trianglesPerPatch = std::max<uint32_t>(1, patchSize);
        triCount = static_cast<uint32_t>(modelMesh->indices.size() / 3);

        patchIdToTriangleId.resize(triCount);
        vertexToPatchIds.resize(modelMesh->vertices.size(), std::vector<uint32_t>{});
        patches.clear();
        patches.reserve((triCount + trianglesPerPatch - 1) / trianglesPerPatch);

        for (uint32_t i = 0; i < triCount; i += trianglesPerPatch)
        {
            const uint32_t patchIdx = static_cast<uint32_t>(patches.size());
            const uint32_t trianglesInPatch = std::min<uint32_t>(trianglesPerPatch, triCount - i);
            float area{};
            glm::vec3 center{};
            glm::vec3 normal{};

            for (uint32_t j = 0; j < trianglesInPatch; ++j)
            {
                const uint32_t tri = i + j;
                uint32_t i0 = modelMesh->indices[3 * tri + 0];
                uint32_t i1 = modelMesh->indices[3 * tri + 1];
                uint32_t i2 = modelMesh->indices[3 * tri + 2];

                const auto& v0 = modelMesh->vertices[i0];
                const auto& v1 = modelMesh->vertices[i1];
                const auto& v2 = modelMesh->vertices[i2];

                vertexToPatchIds[i0].push_back(patchIdx);
                vertexToPatchIds[i1].push_back(patchIdx);
                vertexToPatchIds[i2].push_back(patchIdx);

                glm::vec3 e1 = v1.pos - v0.pos;
                glm::vec3 e2 = v2.pos - v0.pos;
                glm::vec3 triNormalGeom = glm::cross(e1, e2);
                float triArea = 0.5f * glm::length(triNormalGeom);
                glm::vec3 triCenter = (v0.pos + v1.pos + v2.pos) / 3.0f;

                if (triArea <= 1e-8f)
                {
                    patchIdToTriangleId[tri] = patchIdx;
                    continue;
                }

                const glm::vec3 shadingNormal = glm::normalize(v0.normal + v1.normal + v2.normal);
                if (glm::length(shadingNormal) > 1e-8f && glm::dot(triNormalGeom, shadingNormal) < 0.0f)
                {
                    triNormalGeom = -triNormalGeom;
                }

                area += triArea;
                center += triCenter * triArea; // nie wiem po co tak, weighted center. Duze trójkąty maja większy wpływ na pozycje patcha, wiec to jest pewnie po to
                normal += triNormalGeom;
                patchIdToTriangleId[tri] = patchIdx;
            }

            glm::vec3 worldCenter = glm::vec3(modelTransform * glm::vec4(center / area, 1.0f));
            glm::vec3 worldNormal = glm::normalize(glm::mat3(modelTransform) * normal);

            Patch p{};
            p.id = patchIdx;
            p.area = area;
            p.center = (area > 0.0f) ? worldCenter : glm::vec3(0.0f);
            p.normal = (glm::length(normal) > 0.0f) ? worldNormal : glm::vec3(0.0f);
            p.albedo = material.albedo;
            if(material.type == MaterialType::LIGHT)
            {
                p.emission = 1.0f;
            }
            else
            {
                p.emission = 0.0f;
            }

            p.unshotEnergy = p.albedo * p.emission;
            p.radiosity = glm::vec3(0.0f);

            patches.push_back(p);
        }
    }

    void Model::removeDuplicateVertices()
    {
        for (auto& ver : vertexToPatchIds)
        {
            std::sort(ver.begin(), ver.end());
            ver.erase(std::unique(ver.begin(), ver.end()), ver.end());
        }
    }

    void Model::buildVertexPatchAdjacency()
    {
        const uint32_t vertexCount = static_cast<uint32_t>(vertexToPatchIds.size());
        vertexPatchOffsets.clear();
        vertexPatchIndices.clear();
        // Trzeba dodac jeden zeby miec offset dla ostatniego wierzchołka
        vertexPatchOffsets.resize(vertexCount + 1);
        uint32_t offset = 0;


        // wypelnienie tablicy vertexPatchOffsets offsetami
        for (uint32_t v = 0; v < vertexCount; v++)
        {
            vertexPatchOffsets[v] = offset;
            offset += static_cast<uint32_t>(vertexToPatchIds[v].size());
        }

        vertexPatchOffsets[vertexCount] = offset;

        // rezerwujemy tyle miejsca ile wynosi offset czyli ilosc patchy w sumie
        vertexPatchIndices.clear();
        vertexPatchIndices.resize(offset);
        
        for(const auto& list : vertexToPatchIds)
        {
            vertexPatchIndices.insert(vertexPatchIndices.end(), list.begin(), list.end());
        }
    }

    std::pair<std::vector<VertexRT>, std::vector<uint32_t>> CustomModels::createRectangle(const glm::vec3& color)
    {
        std::vector<VertexRT> vertices = {
            {{-5.0f, 0.0f, -5.0f}, {0.0f, 1.0f, 0.0f}, {color}, {0.0f, 0.0f}},
            {{5.0f, 0.0f, -5.0f}, {0.0f, 1.0f, 0.0f}, {color}, {1.0f, 0.0f}},
            {{5.0f, 0.0f, 5.0f}, {0.0f, 1.0f, 0.0f}, {color}, {1.0f, 1.0f}},
            {{-5.0f, 0.0f, 5.0f}, {0.0f, 1.0f, 0.0f}, {color}, {0.0f, 1.0f}}
        };

        std::vector<uint32_t> indices = {
            0, 1, 2,
            2, 3, 0
        };

        return {vertices, indices};
    }

    std::pair<std::vector<VertexRT>, std::vector<uint32_t>> CustomModels::createCube(const glm::vec3& color)
    {
        std::vector<VertexRT> vertices = {
            {{-1.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {color}, {0.0f, 0.0f}},
            {{1.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {color}, {1.0f, 0.0f}},
            {{1.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {color}, {1.0f, 1.0f}},
            {{-1.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {color}, {0.0f, 1.0f}}
        };

        std::vector<uint32_t> indices = {
            0, 1, 2,
            2, 3, 0
        };

        return {vertices, indices};
    }
}
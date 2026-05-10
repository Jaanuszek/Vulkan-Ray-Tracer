#include "Model.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace VRTR
{
    Model::Model(RendererContext& ctx,
                const std::string& modelPath, 
                const std::string& texturePath,
                const glm::mat4& transform)
        : ctx(ctx), modelTransform(transform)
    {
        VRTR_DEBUG("Creating model from path: {}", modelPath);

        modelName = getModelNameFromPath(modelPath);

        if(!texturePath.empty())
            withTexture = true;

        loadModel(modelPath);
        if(vertexInterpolation)
        {
            weldVertices();
        }
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
        const std::vector<VertexRT>& vertices, 
        const std::vector<uint32_t>& indices,
        const std::vector<Material>& mats, const glm::mat4& transform)
        : ctx(ctx), materials(mats), modelTransform(transform)
    {
        VRTR_DEBUG("Creating model from vertices and indices");

        modelMesh = std::make_unique<mesh>();
        modelMesh->vertices = vertices;
        modelMesh->indices = indices;

        if(vertexInterpolation)
        {
            weldVertices();
        }
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
        std::vector<tinyobj::material_t> mtl_materials;
        std::string warn, err;

        const std::filesystem::path modelPath(path);
        std::string baseDir = modelPath.parent_path().string();
        if (!baseDir.empty() && baseDir.back() != '/')
        {
            baseDir.push_back('/');
        }

        bool ret = tinyobj::LoadObj(&attrib, &shapes, &mtl_materials, &warn, &err, path.c_str(), baseDir.c_str());
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

        std::unordered_map<int, uint32_t> mtlIdToMaterialIdx;
        for (uint32_t id = 0; id < mtl_materials.size(); id++)
        {
            const float emissionMean = (mtl_materials[id].emission[0] + mtl_materials[id].emission[1] + mtl_materials[id].emission[2]) / 3.0f;
            Material mat = {
                .albedo = glm::vec4(
                    mtl_materials[id].diffuse[0], 
                    mtl_materials[id].diffuse[1], 
                    mtl_materials[id].diffuse[2], 
                    1.0f),
                .emission = glm::vec3(
                    mtl_materials[id].emission[0],
                    mtl_materials[id].emission[1],
                    mtl_materials[id].emission[2]),
                .metallic = mtl_materials[id].metallic,
                .roughness = mtl_materials[id].roughness,
                .type = emissionMean > 0 ? MaterialType::LIGHT : MaterialType::ALBEDO,
                .textureIndex = 0
            };
            mtlIdToMaterialIdx[id] = materials.size();
            materials.push_back(mat);
        }

        mesh Mesh{};

        // loop po wszystkich shapeach o i g w pliku .obj
        for (const auto& shape : shapes)
        {
            size_t indexOffset = 0;

            // loop po faceach shape'a, czyli po trójkątach lub innych wielokątach
            for (size_t face = 0; face < shape.mesh.num_face_vertices.size(); face++)
            {
                // ilość face'ów w shapie
                const int fv = shape.mesh.num_face_vertices[face];
                const int materialId = shape.mesh.material_ids[face];

                // Loop po wierzchołkach wszystkich face'ów
                for (int v = 0; v < fv; ++v)
                {
                    const auto& idx = shape.mesh.indices[indexOffset + v];
                    VertexRT vertex{};

                    vertex.pos = {
                        attrib.vertices[3 * idx.vertex_index + 0],
                        attrib.vertices[3 * idx.vertex_index + 1],
                        attrib.vertices[3 * idx.vertex_index + 2]
                    };

                    if (!attrib.normals.empty() && idx.normal_index >= 0)
                    {
                        vertex.normal = {
                            attrib.normals[3 * idx.normal_index + 0],
                            attrib.normals[3 * idx.normal_index + 1],
                            attrib.normals[3 * idx.normal_index + 2]
                        };
                    }

                    if(withTexture && !attrib.texcoords.empty() && idx.texcoord_index >= 0)
                    {
                        vertex.texCoord = {
                            attrib.texcoords[2 * idx.texcoord_index + 0],
                            attrib.texcoords[2 * idx.texcoord_index + 1]
                        };
                    }

                    Mesh.vertices.push_back(vertex);
                    Mesh.indices.push_back(Mesh.indices.size());
                }

                indexOffset += static_cast<size_t>(fv);

                if(face < shape.mesh.material_ids.size())
                {
                    int mtlIdFromFile = shape.mesh.material_ids[face];
                    uint32_t matIdx = mtlIdToMaterialIdx.count(mtlIdFromFile) ? mtlIdToMaterialIdx[mtlIdFromFile] : 0;
                    triangleIdToMaterialId.push_back(matIdx);
                }
            }
        }
        modelMesh = std::make_unique<mesh>(std::move(Mesh));
        loadedFromFile = true;
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
        VRTR_DEBUG("Creating vertex buffer for model: {}", modelName);
        vk::DeviceSize bufferSize = modelMesh->vertices.size() * sizeof(VertexRT);
        vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        auto vmaAllocInfo = getVmaAllocCreateInfo();

        vertexBuffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.vmaAllocator, bufferSize, usage, vmaAllocInfo);
        vertexBuffer->Update(modelMesh->vertices.data(), bufferSize);
    }

    void Model::createIndexBuffer()
    {
        VRTR_DEBUG("Creating index buffer for model: {}", modelName);
        vk::DeviceSize bufferSize = modelMesh->indices.size() * sizeof(uint32_t);
        vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        auto vmaAllocInfo = getVmaAllocCreateInfo();

        indexBuffer = std::make_unique<Buffer>(ctx.logicalDevice, ctx.vmaAllocator, bufferSize, usage, vmaAllocInfo);
        indexBuffer->Update(modelMesh->indices.data(), bufferSize);
    }

    void Model::setGeometryInfo()
    {
        VRTR_DEBUG("Setting geometry info for model: {}", modelName);
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
            glm::vec3 albedoAccum{};
            glm::vec3 emissionAccum{};

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

                if (loadedFromFile)
                {
                    glm::vec3 triEmission(0.0f);
                    glm::vec3 triAlbedo(0.5f);

                    if (tri < triangleIdToMaterialId.size())
                    {
                        const uint32_t matIdx = triangleIdToMaterialId[tri];
                        if (matIdx < materials.size())
                        {
                            triEmission = materials[matIdx].emission;
                            triAlbedo = glm::vec3(materials[matIdx].albedo);
                        }
                    }

                    emissionAccum += triEmission * triArea;
                    albedoAccum += triAlbedo * triArea;
                }
                else
                {
                    const glm::vec3 triAlbedo = (v0.color + v1.color + v2.color) / 3.0f;
                    albedoAccum += triAlbedo * triArea;
                }
                patchIdToTriangleId[tri] = patchIdx;
            }

            glm::vec3 worldCenter = glm::vec3(modelTransform * glm::vec4(center / area, 1.0f));
            glm::vec3 worldNormal = glm::normalize(glm::mat3(modelTransform) * normal);

            Patch p{};
            p.id = patchIdx;
            p.area = area;
            p.center = (area > 0.0f) ? worldCenter : glm::vec3(0.0f);
            p.normal = (glm::length(normal) > 0.0f) ? worldNormal : glm::vec3(0.0f);
            p.albedo = (area > 0.0f) ? (albedoAccum / area) : glm::vec3(0.5f);

            p.unshotEnergy = (area > 0.0f) ? (emissionAccum / area) : glm::vec3(0.0f);
            p.radiosity = glm::vec3(0.0f);

            patches.push_back(p);
        }
    }

    void Model::weldVertices()
    {
        std::unordered_map<VertexKey, uint32_t, VertexKeyHash> uniqueVertices;

        std::vector<VertexRT> newVertices;
        std::vector<uint32_t> newIndices;

        for (uint32_t i = 0; i < modelMesh->indices.size(); ++i)
        {
            uint32_t idx = modelMesh->indices[i];
            const auto& v = modelMesh->vertices[idx];

            VertexKey key{
                v.pos,
                v.normal,
                v.color,
                v.texCoord
            };

            auto it = uniqueVertices.find(key);

            if (it == uniqueVertices.end())
            {
                uint32_t newIndex = static_cast<uint32_t>(newVertices.size());
                uniqueVertices[key] = newIndex;

                newVertices.push_back(v);
                newIndices.push_back(newIndex);
            }
            else
            {
                newIndices.push_back(it->second);
            }
        }

        modelMesh->vertices = std::move(newVertices);
        modelMesh->indices = std::move(newIndices);

        VRTR_INFO("Vertex welding: {} vertices", modelMesh->vertices.size());
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
        vertexPatchIndices.reserve(offset);
        
        for(const auto& list : vertexToPatchIds)
        {
            vertexPatchIndices.insert(vertexPatchIndices.end(), list.begin(), list.end());
        }
    }

    void Model::setTextureIndex(uint32_t index)
    {
        if(!loadedFromFile)
        {
            materials.at(0).textureIndex = index;
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
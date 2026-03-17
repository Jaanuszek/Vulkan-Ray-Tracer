#include "Model.hpp"

// it has to be there!
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace VRTR
{
    Model::Model(RendererContext& ctx,
                const std::string& modelPath, const std::string& texturePath,
                const Material& mat)
        : ctx(ctx), material(mat)
    {
        VRTR_DEBUG("Creating model from path: {}", modelPath);

        modelName = getModelNameFromPath(modelPath);

        if(!texturePath.empty())
            withTexture = true;

        loadModel(modelPath);
        createVertexBuffer();
        createIndexBuffer();
        setGeometryInfo();

        if(withTexture)
        {
            loadTexture(texturePath);
        }
    }

    Model::Model(RendererContext& ctx,
        const std::vector<VertexRT>& vertices, const std::vector<uint32_t>& indices,
        const Material& mat)
        : ctx(ctx), material(mat)
    {
        VRTR_DEBUG("Creating model from vertices and indices");

        modelMesh = std::make_unique<mesh>();
        modelMesh->vertices = vertices;
        modelMesh->indices = indices;
        material = mat;

        createVertexBuffer();
        createIndexBuffer();
        setGeometryInfo();
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

        #ifdef enableRadiosity
            buildPatches();
        #endif
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

    void Model::buildPatches()
    {
        triCount = static_cast<uint32_t>(modelMesh->indices.size() / 3);
        patchIdToTriangleId.resize(triCount);
        patches.clear();
        patches.reserve(triCount);

        for (uint32_t i = 0; i < triCount; i++)
        {
            uint32_t i0 = modelMesh->indices[3 * i + 0];
            uint32_t i1 = modelMesh->indices[3 * i + 1];
            uint32_t i2 = modelMesh->indices[3 * i + 2];

            const auto& v0 = modelMesh->vertices[i0];
            const auto& v1 = modelMesh->vertices[i1];
            const auto& v2 = modelMesh->vertices[i2];

            glm::vec3 e1 = v1.pos - v0.pos; 
            glm::vec3 e2 = v2.pos - v0.pos;
            glm::vec3 n = glm::normalize(glm::cross(e1, e2));

            // glm::length(glm::cross(e1,e2)) to pole równoległoboku rozpiętego na wektorach e1 i e2
            // Zeby uzyskac pole trójkąta trzeba podzielić to przez 2
            float area = 0.5f * glm::length(glm::cross(e1, e2));
            glm::vec3 center = (v0.pos + v1.pos + v2.pos) / 3.0f;

            Patch p{};
            p.id = i;
            p.area = area;
            p.center = center;
            p.normal = n;
            p.albedo = material.albedo;

            patches.push_back(p);
            patchIdToTriangleId[i] = i;
        }
    }

    std::pair<std::vector<VertexRT>, std::vector<uint32_t>> CustomModels::createRectangle()
    {
        std::vector<VertexRT> vertices = {
            {{-5.0f, 0.0f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
            {{5.0f, 0.0f, -5.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
            {{5.0f, 0.0f, 5.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
            {{-5.0f, 0.0f, 5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}
        };

        std::vector<uint32_t> indices = {
            0, 1, 2,
            2, 3, 0
        };

        return {vertices, indices};
    }
}
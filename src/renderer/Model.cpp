#include "Model.hpp"

// it has to be there!
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace VRTR
{
    Model::Model(RendererContext& ctx, const std::string& modelPath, const std::string& texturePath)
        : ctx(ctx)
    {
        modelName = getModelNameFromPath(modelPath);
        loadModel(modelPath);

        if(!texturePath.empty())
        {
            withTexture = true;
            loadTexture(texturePath);
        }
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

    void Model::loadTexture(const std::string &path)
    {
        VRTR_DEBUG("Loading texture from path: {}", path);

        assert(std::filesystem::exists(path));

        texture = std::make_unique<Texture>(ctx, path);
    }

}
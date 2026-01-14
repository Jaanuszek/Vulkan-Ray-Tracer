#include "ModelLoader.hpp"

// it has to be there!
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace VRTR
{
    ModelLoader::ModelLoader(RendererContext& ctx)
        : ctx(ctx)
    {
    }

    mesh ModelLoader::loadModel(const std::string &path)
    {
        VRTR_DEBUG("Loading model from path: {}", path);

        assert(std::filesystem::exists(path));

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

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
                // vertex.texCoord = {
                //     attrib.texcoords[2 * idx.texcoord_index + 0],
                //     attrib.texcoords[2 * idx.texcoord_index + 1]
                // };

                Mesh.vertices.push_back(vertex);
                Mesh.indices.push_back(Mesh.indices.size());
            }
        }
        return Mesh;
    }
}
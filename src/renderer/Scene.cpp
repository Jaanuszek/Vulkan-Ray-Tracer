#include "pch.h"
#include "Scene.hpp"

namespace VRTR
{
    Scene::Scene(RendererContext& ctx) : ctx(ctx)
    {
        asManager = std::make_unique<AccelerationStructureManager>(ctx);
    }
    
    Scene::~Scene()
    {
        VRTR_DEBUG("Destroying scene");
        models.clear();
        asManager.reset();
    }

    void Scene::createScene(const glm::vec3& lightPos)
    {
        VRTR_DEBUG("Creating scene");

        std::string viking_room_path = (CONSTANTS::ASSETS_DIR / "models/viking_room/").string();
        std::string viking_room_model_path = viking_room_path + "model/viking_room.obj";
        std::string viking_room_texture_path = viking_room_path + "textures/viking_room.png";

        importModel(viking_room_model_path, viking_room_texture_path);

        std::string guy_model_path = (CONSTANTS::ASSETS_DIR / "models/guy/model/guy.obj").string();
        std::string guy_model_name = Model::getModelNameFromPath(guy_model_path);

        auto [floorVertices, floorIndices] = CustomModels::createRectangle();
        Material floorMat{
            .albedo = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f),
            .type = MaterialType::METALLIC,
        };

        glm::mat4 floorModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.1f, 0.0f));
        addObject("floor", floorVertices, floorIndices, floorMat, floorModel);

        auto [wallVertices, wallIndices] = CustomModels::createRectangle();
        Material wallMat{
            .albedo = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
            .type = MaterialType::METALLIC,
        };
        glm::mat4 wallModel = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f));
        wallModel = glm::rotate(wallModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        wallModel = glm::translate(wallModel, glm::vec3(0.0f, -10.0f, -2.0f));

        addObject("wall", wallVertices, wallIndices, wallMat, wallModel);

        auto [cubeVertices, cubeIndices] = CustomModels::createCube();

        // for(auto& vert : cubeVertices)
        // {
        //     vert.normal = -vert.normal;
        // }

        glm::mat4 lightObjectModel = glm::translate(glm::mat4(1.0f), lightPos);
        lightObjectModel = glm::scale(lightObjectModel, glm::vec3(0.2f));
        lightSourceTLASIdx = addObject(LIGHT_MODEL_NAME, cubeVertices, cubeIndices, Material{
            .type = MaterialType::LIGHT,
        }, lightObjectModel);

        // To musi byc na końcu
        buildTLAS();
    }

    uint32_t Scene::importModel(const std::string &modelPath, const std::string &texPath)
    {
        // Moze byc model bez tekstury, ale nie moze byc modelu bez modelu XD
        assert(!modelPath.empty() && "Model path cannot be empty!");

        if(std::filesystem::exists(modelPath) == false)
        {
            throw std::runtime_error("Model file does not exist: " + modelPath);
        }

        // Add some random color to model, to simulate it without texture
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_real_distribution<float> dist (0.0f, 1.0f);

        Material mat{
            .albedo = glm::vec4(dist(rng), dist(rng), dist(rng), 1.0f),
            // TODO dodac obsluge PBR
            .type = MaterialType::PBR,
        };

        std::string model_name = Model::getModelNameFromPath(modelPath);
        auto [it, inserted] = models.try_emplace(
            model_name,
            std::make_unique<Model>(
            ctx,
            modelPath,
            texPath,
            mat));

        if(it->second->hasTexture())
        {
            it->second->setTextureIndex(textureIndexCounter++);
        }
        else
        {
            it->second->setTextureIndex(CONSTANTS::MAX_TEXTURES);
        }

        uint32_t blasIndex = asManager->createBLAS(models.at(model_name));

        glm::mat4 rotatedModel = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        uint32_t instanceIndex = asManager->addInstance(blasIndex, rotatedModel);
        modelInstanceOrder.push_back(model_name);
        return instanceIndex;
    }

    uint32_t Scene::addObject(const std::string& objName, const std::vector<VertexRT>& vertices, 
                            const std::vector<uint32_t>& indices, const Material& mat,
                            const glm::mat4& transform)
    {
        if(vertices.empty() || indices.empty())
        {
            VRTR_WARN("Vertices or indices are empty. Object will not be added to the scene.");
            return 0;
        }

        if(models.find(objName) != models.end())
        {
            VRTR_WARN("Object with name '{}' already exists. It will be overwritten.", objName);
        }

        // models[objName] = std::make_unique<Model>(ctx, vertices, indices, mat);
        models.try_emplace(objName, std::make_unique<Model>(ctx, vertices, indices, mat));
        models.at(objName)->setTextureIndex(CONSTANTS::MAX_TEXTURES);
        uint32_t blasIndex = asManager->createBLAS(models.at(objName));
        uint32_t instanceIndex = asManager->addInstance(blasIndex, transform);
        modelInstanceOrder.push_back(objName);
        return instanceIndex;
    }

    void Scene::buildTLAS()
    {
        fillSSBOContainers();

        auto geometryBufferSize = CONSTANTS::MAX_OBJECTS * sizeof(GeometryInfo);
        auto materialBufferSize = CONSTANTS::MAX_OBJECTS * sizeof(Material);
        auto triToPatchBufferSize = triToPatchGlobal.size() * sizeof(uint32_t);
        // auto patchBufferSize = patchesGlobal.size() * sizeof(Patch);

        geometrySBO = std::make_unique<StorageBuffer>(ctx, geometryBufferSize);
        materialSBO = std::make_unique<StorageBuffer>(ctx, materialBufferSize);
        triToPatchBuffer = std::make_unique<StorageBuffer>(ctx, triToPatchBufferSize);
        // patchBuffer = std::make_unique<StorageBuffer>(ctx, patchBufferSize);

        geometrySBO->copyDataToBuffer(geometryInfos.data(), geometryInfos.size() * sizeof(GeometryInfo));
        materialSBO->copyDataToBuffer(materials.data(), materials.size() * sizeof(Material));
        triToPatchBuffer->copyDataToBuffer(triToPatchGlobal.data(), triToPatchGlobal.size() * sizeof(uint32_t));
        // patchBuffer->copyDataToBuffer(patchesGlobal.data(), patchesGlobal.size() * sizeof(Patch));

        asManager->buildTLAS();
    }

    void Scene::updateTLAS(const float& rotationAngle)
    {
        asManager->updateTLAS(rotationAngle);
    }

    void Scene::updateInstanceTLAS(uint32_t instanceIdx, const glm::mat4& newTransform)
    {
        asManager->updateInstanceTransform(instanceIdx, newTransform);
    }

    void Scene::appendDescriptorResources(DescriptorResources& resources)
    {
        resources.TLAS = asManager->getTLASHandle();
        resources.texImageViews.clear();
        resources.texSamplers.clear();

        for (const auto& modelName : modelInstanceOrder)
        {
            const auto& model = models.at(modelName);
            if (model->hasTexture())
            {
                resources.texImageViews.push_back(model->getTexture().getTextureImageViewHandle());
                resources.texSamplers.push_back(model->getTexture().getTextureSamplerHandle());

                // Aktualnie obsluguje tylko jedną teksturę
                break;
            }
        }

        resources.geometryInfoBuffer = geometrySBO->getBufferHandle();
        resources.materialBuffer = materialSBO->getBufferHandle();
        resources.triToPatchBuffer = triToPatchBuffer->getBufferHandle();
        // resources.patchBuffer = patchBuffer->getBufferHandle();
    }

    void Scene::updatePatchData(uint8_t patchSize)
    {
        for (auto& modelEntry : models)
        {
            modelEntry.second->rebuildPatches(patchSize);
        }

        fillSSBOContainers();

        triToPatchBuffer->copyDataToBuffer(triToPatchGlobal.data(), triToPatchGlobal.size() * sizeof(uint32_t));
        // patchBuffer->copyDataToBuffer(patchesGlobal.data(), patchesGlobal.size() * sizeof(Patch));
    }

    void Scene::fillSSBOContainers()
    {
        triToPatchGlobal.clear();
        patchesGlobal.clear();
        geometryInfos.clear();
        materials.clear();
        geometryInfos.reserve(models.size());
        materials.reserve(models.size());

        uint32_t globalPatchBase = 0;
        uint32_t triToPatchOffset = 0;

        for (const auto& modelName : modelInstanceOrder)
        {
            if (models.find(modelName) == models.end())
            {
                throw std::runtime_error("Model missing for TLAS instance order: " + modelName);
            }

            const auto& model = models.at(modelName);

            GeometryInfo gi = model->getGeometryInfo();
            gi.triToPatchOffset = triToPatchOffset;
            gi.triangleCount = static_cast<uint32_t>(model->getTriangleCount());
            geometryInfos.push_back(gi);

            materials.push_back(model->getMaterial());

            for(uint32_t localPatchId : model->getPatchIdToTriangleId())
            {
                triToPatchGlobal.push_back(globalPatchBase + localPatchId);
            }

            for(const auto& patchLocal : model->getPatches())
            {
                Patch p = patchLocal;
                p.id = globalPatchBase + patchLocal.id;
                patchesGlobal.push_back(p);
            }

            triToPatchOffset += gi.triangleCount;
            globalPatchBase += static_cast<uint32_t>(model->getPatches().size());
        }
    }
}
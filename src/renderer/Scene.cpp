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
        // std::string viking_room_model_path = viking_room_path + "model/viking_room.obj";
        std::string viking_room_texture_path = viking_room_path + "textures/viking_room.png";
        std::string viking_room_model_path = viking_room_path + "model/vikin_house_20SubDivision.obj";

        glm::mat4 rotatedModel = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        // importModel(viking_room_model_path, viking_room_texture_path, rotatedModel);

        std::string guy_model_path = (CONSTANTS::ASSETS_DIR / "models/guy/model/guy.obj").string();
        std::string guy_model_name = Model::getModelNameFromPath(guy_model_path);

        std::string cornell_box_path = (CONSTANTS::ASSETS_DIR / "models/cornell_box/").string();
        std::string cornell_box_model_path = cornell_box_path + "model/cornell_box_sub20.obj";
        glm::mat4 cornellBoxModel = glm::scale(glm::mat4(1.0f), glm::vec3(0.2f));
        importModel(cornell_box_model_path, "", cornellBoxModel);

        // auto [floorVertices, floorIndices] = CustomModels::createRectangle();
        // Material floorMat{
        //     .albedo = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f),
        //     .type = MaterialType::METALLIC,
        // };

        // glm::mat4 floorModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.1f, 0.0f));
        // addObject("floor", floorVertices, floorIndices, floorMat, floorModel);

        auto [wallVertices, wallIndices] = CustomModels::createRectangle(glm::vec3(0.0f, 1.0f, 0.0f));
        Material wallMat{
            .albedo = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
            .type = MaterialType::LIGHT,
        };
        glm::mat4 wallModel(1.0f);
        wallModel = glm::translate(wallModel, glm::vec3(0.0f, 0.5f, -1.0f));
        wallModel = glm::rotate(wallModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        wallModel = glm::scale(wallModel, glm::vec3(0.1f));

        // addObject("wall", wallVertices, wallIndices, wallMat, wallModel);

        auto [wallVertices2, wallIndices2] = CustomModels::createRectangle(glm::vec3(1.0f, 0.0f, 0.0f));
        Material wallMat2{
            .albedo = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
            .type = MaterialType::LIGHT,
        };
        glm::mat4 wallModel2(1.0f);
        wallModel2 = glm::translate(wallModel2, glm::vec3(1.0f, 0.5f, 0.0f));
        wallModel2 = glm::rotate(wallModel2, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        wallModel2 = glm::scale(wallModel2, glm::vec3(0.1f));

        // addObject("wall2", wallVertices2, wallIndices2, wallMat2, wallModel2);

        auto [cubeVertices, cubeIndices] = CustomModels::createCube(glm::vec3(0.0f, 0.0f, 1.0f));

            auto addLightCube = [&](const std::string& name, const glm::vec3& pos, const glm::vec3& scale, const glm::vec4& color)
            {
                glm::mat4 model(1.0f);
                model = glm::translate(model, pos);
                model = glm::scale(model, scale);
                std::vector<Material> mats{
                    Material{
                        .albedo = color,
                        .type = MaterialType::LIGHT,
                    }
                };
                return addObject(name, cubeVertices, cubeIndices, mats, model);
            };

            // Glowne zrodlo sterowane przez GUI (Light Position).
            // lightSourceTLASIdx = addLightCube("light_main", lightPos, glm::vec3(0.25f), glm::vec4(1.0f));

            // Dodatkowe wypelniajace zrodla, z unikalnymi nazwami i transformacjami.
            // addLightCube("light_fill_left", glm::vec3(0.0f, 1.0f, -0.1f), glm::vec3(0.7f), glm::vec4(0.0f,0.0f,1.0f,1.0f));
            // addLightCube("light_fill_right", glm::vec3(0.5f, 1.0f, -0.1f), glm::vec3(0.18f), glm::vec4(0.0f,0.0f,1.0f,1.0f));
            // addLightCube("light_fill_front", glm::vec3(0.0f, 1.0f, 0.3f), glm::vec3(0.14f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));

        // To musi byc na końcu
            buildTLAS();
            std::cout << "Patch size: " << sizeof(Patch) * patchesGlobal.size() << " bytes" << std::endl;
    }

    uint32_t Scene::importModel(const std::string &modelPath, const std::string &texPath, const glm::mat4& transform)
    {
        // Moze byc model bez tekstury, ale nie moze byc modelu bez modelu XD
        assert(!modelPath.empty() && "Model path cannot be empty!");

        if(std::filesystem::exists(modelPath) == false)
        {
            throw std::runtime_error("Model file does not exist: " + modelPath);
        }

        std::string model_name = Model::getModelNameFromPath(modelPath);
        auto [it, inserted] = models.try_emplace(
            model_name,
            std::make_unique<Model>(
            ctx,
            modelPath,
            texPath,
            transform));

        if(it->second->hasTexture())
        {
            it->second->setTextureIndex(textureIndexCounter++);
        }
        else
        {
            it->second->setTextureIndex(CONSTANTS::MAX_TEXTURES);
        }
        uint32_t blasIndex = asManager->createBLAS(models.at(model_name));

        uint32_t instanceIndex = asManager->addInstance(blasIndex, transform);
        modelInstanceOrder.push_back(model_name);
        return instanceIndex;
    }

    uint32_t Scene::addObject(const std::string& objName, const std::vector<VertexRT>& vertices, 
                            const std::vector<uint32_t>& indices, const std::vector<Material>& mats,
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
        models.try_emplace(objName, std::make_unique<Model>(ctx, vertices, indices, mats, transform));
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
        auto triToMaterialIdBufferSize = triToMaterialIdGlobal.size() * sizeof(uint32_t);
        // auto patchBufferSize = patchesGlobal.size() * sizeof(Patch);

        geometrySBO = std::make_unique<StorageBuffer>(ctx, geometryBufferSize);
        materialSBO = std::make_unique<StorageBuffer>(ctx, materialBufferSize);
        triToPatchBuffer = std::make_unique<StorageBuffer>(ctx, triToPatchBufferSize);
        triToMaterialIdBuffer = std::make_unique<StorageBuffer>(ctx, triToMaterialIdBufferSize);
        // patchBuffer = std::make_unique<StorageBuffer>(ctx, patchBufferSize);

        geometrySBO->copyDataToBuffer(geometryInfos.data(), geometryInfos.size() * sizeof(GeometryInfo));
        materialSBO->copyDataToBuffer(materials.data(), materials.size() * sizeof(Material));
        triToPatchBuffer->copyDataToBuffer(triToPatchGlobal.data(), triToPatchGlobal.size() * sizeof(uint32_t));
        triToMaterialIdBuffer->copyDataToBuffer(triToMaterialIdGlobal.data(), triToMaterialIdGlobal.size() * sizeof(uint32_t));
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
        resources.triToMaterialIdBuffer = triToMaterialIdGlobal.empty() ? nullptr : triToMaterialIdBuffer->getBufferHandle();
    }

    void Scene::updatePatchData(uint8_t patchSize)
    {
        for (auto& modelEntry : models)
        {
            modelEntry.second->rebuildPatches(patchSize);
        }

        fillSSBOContainers();

        triToPatchBuffer->copyDataToBuffer(triToPatchGlobal.data(), triToPatchGlobal.size() * sizeof(uint32_t));
        triToMaterialIdBuffer->copyDataToBuffer(triToMaterialIdGlobal.data(), triToMaterialIdGlobal.size() * sizeof(uint32_t));
        // patchBuffer->copyDataToBuffer(patchesGlobal.data(), patchesGlobal.size() * sizeof(Patch));
    }

    void Scene::fillSSBOContainers()
    {
        triToPatchGlobal.clear();
        patchesGlobal.clear();
        geometryInfos.clear();
        materials.clear();
        vertexToPatchGlobal.clear();
        vertexToPatchOffsetGlobal.clear();
        triToMaterialIdGlobal.clear();
        
        geometryInfos.reserve(models.size());
        materials.reserve(models.size());
        triToMaterialIdGlobal.reserve(getVertexCount());
        uint32_t globalPatchBase = 0;
        uint32_t triToPatchOffset = 0; // offset trójkąta w globalnym kontenerze triToPatchGlobal - który mapuje indekst trójkąta na indeks patcha
        uint32_t globalIndexBase = 0;  // całkowita liczba indeksów patchy już dodanych (dla CSR)
        uint32_t globalVertexBase = 0; // globalny offset wierzchołków dla modelu (indeks do vertexRadiosityBuffer)
        uint32_t globalModelBase = 0;

        for (const auto& modelName : modelInstanceOrder)
        {
            if (models.find(modelName) == models.end())
            {
                throw std::runtime_error("Model missing for TLAS instance order: " + modelName);
            }

            const auto& model = models.at(modelName);

            GeometryInfo gi = model->getGeometryInfo();
            gi.triToPatchOffset = triToPatchOffset; // jeden na model
            gi.triangleCount = static_cast<uint32_t>(model->getTriangleCount());
            gi.vertexGlobalOffset = globalVertexBase;
            gi.MaterialGlobalOffset = globalModelBase;
            geometryInfos.push_back(gi);

            // Dodaj wszystkiem aterialy z danego modelu
            const auto& modelMaterials = model->getMaterials();
            materials.insert(materials.end(), modelMaterials.begin(), modelMaterials.end());

            // Przechodzimy przez kontener TRI -> PatchId: arr[tri] = patchId 
            for(uint32_t localPatchId : model->getPatchIdToTriangleId())
            {
                // triToPatchGlobal[tri] = globalPatchId
                triToPatchGlobal.push_back(globalPatchBase + localPatchId);
            }

            const auto& modelPatches = model->getPatches();
            const auto& localTriToMaterialId = model->getTriIdxToMaterialIdx();
            // Przechodzimy po kontenerze przechowującym wszystkie lokalne patche modelu
            for (size_t patchIdx = 0; patchIdx < modelPatches.size(); patchIdx++)
            {
                Patch p = modelPatches[patchIdx];
                uint32_t patchId = p.id;
                // podmianka id z lokalnej na globalną
                p.id = globalPatchBase + patchId;
                patchesGlobal.push_back(p);
            }

            // z jakiegos powodu mam mniej patchy niż trójkątów??
            for (size_t i = 0; i < localTriToMaterialId.size(); i++)
            {
                triToMaterialIdGlobal.push_back(globalModelBase + localTriToMaterialId[i]);
            }

            // Budowanie globalnej adjacency: vertex -> patch mapping (CSR format)
            const auto &localVertexToPatchIds = model->getLocalVertexToPatchIds();
            const auto& localVertexToPatchOffsets = model->getLocalVertexToPatchOffsets();
            uint32_t vertexCount = static_cast<uint32_t>(model->getVertexCount());

            // Dodaj wszystkie indeksy patchy dla wierzchołków tego modelu
            for (uint32_t localPatchId : localVertexToPatchIds)
            {
                vertexToPatchGlobal.push_back(globalPatchBase + localPatchId);
            }

            // Dodaj offsety dla każdego wierzchołka
            for (uint32_t v = 0; v < vertexCount; ++v)
            {
                vertexToPatchOffsetGlobal.push_back(globalIndexBase + localVertexToPatchOffsets[v]);
            }

            triToPatchOffset += gi.triangleCount;
            globalPatchBase += static_cast<uint32_t>(modelPatches.size());
            globalIndexBase += static_cast<uint32_t>(localVertexToPatchIds.size());
            globalVertexBase += vertexCount;
            globalModelBase += static_cast<uint32_t>(modelMaterials.size());
        }

        // Dodaj sentinel na koniec CSR - wskazuje za ostatni indeks
        vertexToPatchOffsetGlobal.push_back(globalIndexBase);
    }

    uint32_t Scene::getVertexCount() const
    {
        uint32_t vertexCount = 0;
        for (const auto& [name, model] : models)
        {
            vertexCount += static_cast<uint32_t>(model->getVertexCount());
        }
        return vertexCount;
    }
}
#include <pch.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "init_cuda.cuh"

#include "Engine.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
static vk::detail::DynamicLoader dl;

// Start resolution
// On window resize, those values will not be updated here, but inside RTRenderer
constexpr unsigned int WIDTH = 2560;
constexpr unsigned int HEIGHT = 1440;

const std::string SPONZA_MODEL_PATH = "assets/models/isometric_room/subdivided_sponza.obj";
const std::string JAPANESE_MODEL_PATH = "assets/models/isometric_room/japanese_subdivide_bigger.obj";
const std::string NVIDIA_SCENE_MODEL_PATH = "assets/models/isometric_room/nvidiaScene_sub_separated.obj";
const std::string ROOM_FIREPLACE_MODEL_PATH = "assets/models/isometric_room/room_fireplace_walls.obj";
const std::string MY_CORNELL_BOX_MODEL_PATH = "assets/models/isometric_room/myCornellBoxobj.obj";
const std::string CLASSROOM_MODEL_PATH = "assets/models/isometric_room/classRoom_30_adj2.obj";
const std::string CORNELL_BOX_MODEL_PATH = "assets/models/cornell_box/model/cornell_box_sub20.obj";

double lastFrameTime{};
double deltaTime{};

constexpr uint32_t FrameToCapture = 50000;

int main()
{
    try
    {
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        // Engine init
        VRTR::Engine engine;
        engine.init(WIDTH, HEIGHT);

        // Scene initialization
        glm::mat4 modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));

        engine.addModel(CORNELL_BOX_MODEL_PATH, "", modelMat);

        auto [floorVertices, floorIndices] = VRTR::CustomModels::createDividedRectangle(50, 50, 1.0f, 1.0f, glm::vec3(0.8f, 0.8f, 0.8f));
        VRTR::Material floorMat{
            .albedo = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f),
            .metallic = 1.0f,
            .roughness = 0.0f,
            .type = VRTR::MaterialType::METALLIC,
        };
        glm::mat4 floorModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 3.0f, 0.0f));
        floorModel = glm::scale(floorModel, glm::vec3(10.0f));
        floorModel = glm::rotate(floorModel, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        engine.addMesh("floor", floorVertices, floorIndices, {floorMat}, floorModel);

        // auto [sphereVertices, sphereIndices] = VRTR::CustomModels::createSphere(32, 64, 1.0f, glm::vec3(0.95f, 0.97f, 1.0f));
        // VRTR::Material sphereMat{
        //     .albedo = glm::vec4(0.95f, 0.97f, 1.0f, 1.0f),
        //     .emission = glm::vec3(0.0f),
        //     .metallic = 0.0f,
        //     .roughness = 0.0f,
        //     .type = VRTR::MaterialType::REFRACTION,
        // };
        // glm::mat4 sphereModel(1.0f);
        // sphereModel = glm::translate(sphereModel, glm::vec3(0.0f, 2.5f, -2.0f));
        // sphereModel = glm::scale(sphereModel, glm::vec3(0.3f));
        // engine.addMesh("glass_sphere", sphereVertices, sphereIndices, {sphereMat}, sphereModel);

        // auto [squareVertices, squareIndices] = VRTR::CustomModels::createDividedCube(10, 10, 10, 1.0f, glm::vec3(1.0f, 0.0f, 0.0f));
        // VRTR::Material squareMat{
        //     .albedo = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
        //     .emission = glm::vec3(0.0f),
        //     .metallic = 1.0f,
        //     .roughness = 0.0f,
        //     .type = VRTR::MaterialType::ALBEDO,
        // };
        // glm::mat4 squareModel(1.0f);
        // squareModel = glm::translate(squareModel, glm::vec3(1.0f, 2.5f, -4.5f));
        // squareModel = glm::rotate(squareModel, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        // squareModel = glm::scale(squareModel, glm::vec3(1.5f));
        // engine.addMesh("red_square", squareVertices, squareIndices, {squareMat}, squareModel);

        // Build scene, resources and run infinite program loop
        engine.buildScene();
        engine.initResources();
        engine.runLoop();
    }
    catch (const std::exception& e)
    {
        VRTR_CRITICAL("Exception: {}", e.what());
        return EXIT_FAILURE;
    }

    return 0;
}

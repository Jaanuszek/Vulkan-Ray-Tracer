#pragma once

#include <memory>
#include "RTRenderer.hpp"
#include "Camera.hpp"
#include "SceneSettings.hpp"
#include "InputManager.hpp"

namespace VRTR
{
    class Engine
    {
    public:
        Engine();
        ~Engine();

        void init(int width = 1280, int height = 720);
        void initResources();

        uint32_t addModel(const std::string& modelPath, const std::string& texPath, const glm::mat4& transform = glm::mat4(1.0f));
        uint32_t addMesh(const std::string& name, const std::vector<VertexRT>& vertices, const std::vector<uint32_t>& indices, const std::vector<Material>& mats, const glm::mat4& transform = glm::mat4(1.0f));
        void buildScene();

        void setInstanceTransform(uint32_t instanceIdx, const glm::mat4& newTransform);
        void rotateScene(float rotationAngle);
        void setLightPosition(const glm::vec3& pos);
        void setPatchSize(uint8_t size);

        void runLoop();

    private:
        GLFWwindow* window{nullptr};
        SceneSettings sceneSettings;
        std::shared_ptr<Camera> camera;
        std::unique_ptr<RTRenderer> renderer;
        int width{1280}, height{720};
    };
}

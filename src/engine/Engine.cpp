#include "Engine.hpp"
#include "pch.h"

namespace VRTR
{
    Engine::Engine()
    {
    }

    Engine::~Engine()
    {
        renderer.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        VRTR::Logger::destroy();
    }

    void Engine::init(int width, int height)
    {
        VRTR::Logger::init();
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        if (!primaryMonitor) {
            VRTR_CRITICAL("Failed to get primary monitor");
        }

        this->width = width;
        this->height = height;

        this->window = glfwCreateWindow(width, height, "VRTR", primaryMonitor, nullptr);

        sceneSettings = SceneSettings{};
        camera = std::make_shared<Camera>(sceneSettings, glm::vec3(0.0f, 0.0f, 4.0f));
        camera->setPerspective(60.0f, static_cast<float>(width) / height, 0.1f, 100.0f);

        renderer = std::make_unique<RTRenderer>(camera, sceneSettings);
        renderer->init(window, false);

        glfwSetWindowUserPointer(window, camera.get());
        glfwSetFramebufferSizeCallback(window, VRTR::framebufferResizeCallback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        VRTR::InputManager::init(window);
    }

    void Engine::initResources()
    {
        renderer->buildResources(window);
    }

    uint32_t Engine::addModel(const std::string& modelPath, const std::string& texPath, const glm::mat4& transform)
    {
        if(!renderer) throw std::runtime_error("Engine not initialized");
        return renderer->addModel(modelPath, texPath, transform);
    }

    uint32_t Engine::addMesh(const std::string& name, const std::vector<VertexRT>& vertices, const std::vector<uint32_t>& indices, const std::vector<Material>& mats, const glm::mat4& transform)
    {
        if(!renderer) throw std::runtime_error("Engine not initialized");
        return renderer->addMesh(name, vertices, indices, mats, transform);
    }

    void Engine::buildScene()
    {
        if(!renderer) throw std::runtime_error("Engine not initialized");
        renderer->buildTLAS();
    }

    void Engine::setInstanceTransform(uint32_t instanceIdx, const glm::mat4& newTransform)
    {
        if(!renderer) throw std::runtime_error("Engine not initialized");
        renderer->setInstanceTransform(instanceIdx, newTransform);
    }

    void Engine::rotateScene(float rotationAngle)
    {
        if(!renderer) throw std::runtime_error("Engine not initialized");
        renderer->rotateScene(rotationAngle);
    }

    void Engine::setLightPosition(const glm::vec3& pos)
    {
        if(!renderer) throw std::runtime_error("Engine not initialized");
        renderer->setLightPosition(pos);
    }

    void Engine::setPatchSize(uint8_t size)
    {
        if(!renderer) throw std::runtime_error("Engine not initialized");
        renderer->setPatchSize(size);
    }

    void Engine::runLoop()
    {
        if(!renderer) throw std::runtime_error("Engine not initialized");

        double lastFrameTime = 0.0;
        while(!glfwWindowShouldClose(window))
        {
            double currentTime = glfwGetTime();
            double deltaTime = currentTime - lastFrameTime;
            lastFrameTime = currentTime;
            glfwPollEvents();
            VRTR::processInput(window, deltaTime, camera);
            renderer->drawFrame(window, deltaTime, VRTR::InputManager::renderGUI);
        }
    }
}

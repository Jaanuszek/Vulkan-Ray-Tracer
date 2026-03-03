#pragma once
#include <Logger.hpp>
#include "SceneSettings.hpp"

namespace VRTR
{
    constexpr float YAW = -90.0f;
    constexpr float PITCH = 0.0f;
    constexpr float camSpeed = 2.5f;
    constexpr float SENSITIVITY = 0.1f;

    class CORE_EXPORT Camera
    {
    public:
        Camera(SceneSettings &sceneSettings, const glm::vec3& pos);
        ~Camera() = default;

        struct Matrices
        {
            glm::mat4 view;
            glm::mat4 perspective;
        } matrices;

        void setPerspective(float fov, float aspect, float near, float far);

        void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

        void moveForward(float distance);
        void moveRight(float distance);
        void moveUp(float distance);

    private:
        void updateViewMatrix();
        void updateCameraVectors();

    private:
        SceneSettings &sceneSettings;
        float fov{};
        float near{}, far{};
        glm::vec3 rotation{};
        glm::vec3 translation{};

        glm::vec3 pos{0.0f, 0.0f, 4.0f};
        glm::vec3 front{0.0f, 0.0f, 1.0f};
        glm::vec3 up{0.0f, 1.0f, 0.0f};
        glm::vec3 right{};
        glm::vec3 worldUp{0.0f, 1.0f, 0.0f};
        float yaw = YAW;
        float pitch = PITCH;
    };
}
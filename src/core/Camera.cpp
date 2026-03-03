#include "Camera.hpp"
#include "pch.h"

namespace VRTR
{
    Camera::Camera(SceneSettings &sceneSettings, const glm::vec3& pos) : sceneSettings(sceneSettings), pos(pos)
    {
        updateCameraVectors();
        updateViewMatrix();
    }

    void Camera::setPerspective(float fov, float aspect, float near, float far)
    {
        this->fov = fov;
        this->near = near;
        this->far = far;

        matrices.perspective = glm::perspective(glm::radians(fov), aspect, near, far);
        matrices.perspective[1][1] *= -1; // Invert Y for Vulkan
        sceneSettings.ubo.proj_inverse = glm::inverse(matrices.perspective);
    }
    
    void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
    {
        xoffset *= SENSITIVITY;
        yoffset *= SENSITIVITY;

        yaw += xoffset;
        yaw = std::fmod(yaw, 360.0f); // Keep yaw in the range [0, 360)
        pitch -= yoffset; // Invert yoffset to match typical camera controls

        if (constrainPitch)
        {
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
        }

        updateCameraVectors();
        updateViewMatrix();
    }

    void Camera::updateViewMatrix()
    {
        matrices.view = glm::lookAt(pos, pos + front, up);
        sceneSettings.ubo.view_inverse = glm::inverse(matrices.view);
    }

    void Camera::updateCameraVectors()
    {
        glm::vec3 frontVec;
        frontVec.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        frontVec.y = sin(glm::radians(pitch));
        frontVec.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(frontVec);
        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));
    }

    void Camera::moveForward(float distance)
    {
        pos += front * distance;
        updateViewMatrix();
    }

    void Camera::moveRight(float distance)
    {
        pos += right * distance;
        updateViewMatrix();
    }

    void Camera::moveUp(float distance)
    {
        pos += worldUp * distance;
        updateViewMatrix();
    }
}
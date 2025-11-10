#include "Camera.hpp"
#include "pch.h"

namespace VRTR
{
    void Camera::setPerspective(float fov, float aspect, float near, float far)
    {
        this->fov = fov;
        this->near = near;
        this->far = far;

        matrices.perspective = glm::perspective(glm::radians(fov), aspect, near, far);
        // matrices.perspective[1][1] *= -1; // Invert Y for Vulkan
    }

    void Camera::setRotation(const glm::vec3 &rotation)
    {
        this->rotation = rotation;
        updateViewMatrix();
    }

    void Camera::setTranslation(const glm::vec3 &translation)
    {
        this->translation = translation;
        updateViewMatrix();
    }

    void Camera::updateViewMatrix()
    {
        glm::mat4 rotationMatrix = glm::mat4(1.0f);
        glm::mat4 transformation_matrix{};

        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        transformation_matrix = glm::translate(glm::mat4(1.0f), translation);

        matrices.view = transformation_matrix * rotationMatrix;
    }
}
#pragma once
#include <Logger.hpp>

namespace VRTR
{
    class Camera
    {
        public:
            Camera() = default;
            ~Camera() = default;

            struct Matrices
            {
                glm::mat4 view;
                glm::mat4 perspective;
            } matrices;

            void setPerspective(float fov, float aspect, float near, float far);

            void setRotation(const glm::vec3& rotation);

            void setTranslation(const glm::vec3& translation);

        private:
            float fov{};
            float near{}, far{};
            glm::vec3 rotation{};
            glm::vec3 translation{};

            void updateViewMatrix();

    };
}
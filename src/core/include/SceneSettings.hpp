#pragma once
#include "pch.h"
// tutaj bedą przechowywne struktury, ktore beda w Rendererze i GUI
// Struktury beda przechowywac dane ktore są:
// Modyfikowane w GUI
// Konsumowane w Rendererze

struct UniformData
{
    alignas(4) bool enableCUDA = false; // bool w std140 jest traktowany jako 4 bajty xd
    alignas(4) bool shadowMode = false;
    glm::vec3 light_pos;
    glm::mat4 view_inverse;
    glm::mat4 proj_inverse;
};

struct SceneTransformations
{
    float rotationAngle{0.0f};
};

struct SceneSettings
{
    UniformData ubo;
    SceneTransformations transformations;
};
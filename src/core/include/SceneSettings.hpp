#pragma once
#include "pch.h"
// tutaj bedą przechowywne struktury, ktore beda w Rendererze i GUI
// Struktury beda przechowywac dane ktore są:
// Modyfikowane w GUI
// Konsumowane w Rendererze

enum class UpdateRequest
{
    None = 0,
    Rotation = 1 << 0,
    LightPos = 1 << 1,
    PatchTriangleSize = 1 << 2,
    EnableRadiosityPass = 1 << 3,
};

struct UniformData
{
    alignas(4) bool enableCUDA = false; // bool w std140 jest traktowany jako 4 bajty xd
    alignas(4) bool shadowMode = false;
    alignas(4) bool debugPatches = false; // pokazuje kolorami patche sceny
    alignas(4) bool debugNormals = false; // pokazuje normalne patchy
    alignas(4) bool debugCenters = false; // pokazuje centra patchy
    alignas(4) bool useRadiosityLightmap = false;
    alignas(4) bool enableRadiosityPass = false;
    glm::vec3 light_pos = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 view_inverse;
    glm::mat4 proj_inverse;
};

struct SceneTransformations
{
    // bool RotationChange = false;
    // bool LightPosChange = false;
    // bool PatchTriangleSizeChange = false;
    UpdateRequest updateRequest = UpdateRequest::None;
    float rotationAngle{0.0f};
    int patchTriangleSize{1};
};

struct SceneSettings
{
    UniformData ubo;
    SceneTransformations transformations;
};
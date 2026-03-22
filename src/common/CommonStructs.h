#pragma once

namespace VRTR
{
    struct Patch
    {
        uint32_t id;
        float area;
        float emission{0.0f}; // wartosc z przedmialu [0,1]
        glm::vec3 center;
        glm::vec3 normal;
        glm::vec3 albedo;
        
        // RADIOSITY - energia wysłana i otrzymana
        float unshotEnergy{0.0f};      // energia, ktora jeszcze nie byla radiosity
        glm::vec3 radiosity{0.0f, 0.0f, 0.0f};
    };

    // Struktura ktora bedzie uzupelniona w visability passie
    struct PatchVisibility
    {
        uint32_t srcPatchId;
        uint32_t dstPatchId;
        float visibility;              // wynik visibility: 0.0 - 1.0 (0 = niewidoczny)
    };

    // Konfiguracja parametrow radiosity
    struct RadiosityConfig
    {
        uint32_t maxIterations;
        float convergenceThreshold;    // gdy unshotEnergy < threshold, konczym
        float hemicubeResolution;      // rozdzielczosc hemicube'a dla visibility
    };

    // Rezultat z kernel filtrowania - wybrany patch do radiosity
    struct SelectedPatch
    {
        uint32_t patchId;
        float unshotEnergy;
    };
}
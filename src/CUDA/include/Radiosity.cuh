#pragma once

#include <cuda_runtime_api.h>
#include "CommonStructs.h"
#include <cccl/thrust/host_vector.h>


// Poniewaz mam problemy z linkerem bo krzyczy o libke CORE gdy includuje Logger.hpp, 
// wiec narazie robie workaround i wypisuje error to stderr 
#define CUDA_CHECK_STD_ERROR(err) checkCudaStdError(err,  #err, __FILE__, __LINE__)

inline void checkCudaStdError(cudaError_t err, const char* msg, const char* file, const int line)
{
    if (err != cudaSuccess)
    {
        std::cerr << "CUDA Error: " << msg << ": " << cudaGetErrorString(err) << " from file " << file << ", line " << line << std::endl;
        exit(EXIT_FAILURE);
    }
}


namespace VRTR
{
    namespace CUDA
    {
        constexpr uint32_t TPB = 1024;

        constexpr uint32_t SELECTED_PATCHES_COUNT = 128;

        __global__ void filterPatches(Patch *patches, uint32_t numPatches,
                                      const SelectedPatch* alreadySelectedPatches,
                                      uint32_t alreadySelectedCount,
                                      SelectedPatch* selectedPatch);

        __global__ void reduceSelectedPatches(SelectedPatch *input, uint32_t n, SelectedPatch *output);

        /**
         * @brief Kernel do obliczenia radiosity na bazie visibility
         * @param patches Array patchy
         * @param numPatches Ilość patchy
         * @param visibilities Array visibility (results z Vulkan ray tracingu)
         * @param numVisibilities Ilość vidibilities
         * @param srcPatchId Patch, z ktorego wysylamy radiosity
         */
        __global__ void calculateRadiosity(Patch *patches, uint32_t numPatches,
                                          PatchVisibility *visibilities, uint32_t numVisibilities,
                                          const SelectedPatch* selectedPatch,
                                          const uint32_t* sourceHitCounts,
                                          float4* d_lightMap);

        // Kernel odpowiadający za interpolacje kolorów wierzchołków
        // Bierze patche przypisane do danego wierzchołka i robi średnią ich kolorów
        __global__ void interpolateVertexColors(Patch* patches, uint32_t numPatches,
                                                const uint32_t* vertexPatchIndices, const uint32_t* vertexPatchOffsets,
                                                uint32_t numVertices, glm::vec3* radVertexColors);

        __host__ void runFilterPatchesKernel(Patch* d_patches, uint32_t numPatches, SelectedPatch* d_selectedPatch, cudaStream_t stream);
        __host__ void runPostVisibilityKernel(Patch* d_patches, uint32_t numPatches, 
                                                SelectedPatch* d_selectedPatch, PatchVisibility* d_visibilities, 
                                                uint32_t numVisibilities, float4* d_lightMap, cudaStream_t stream);
        // __host__ void runPostVisibilityKernelStub(Patch* d_patches, uint32_t numPatches, SelectedPatch* d_selectedPatch, cudaStream_t stream);
        __host__ void runInterpolateVertexKernel(Patch* d_patches, uint32_t numPatches,
                                            const uint32_t* d_vertexPatchIndices, const uint32_t* d_vertexPatchOffsets,
                                            uint32_t numVertices, glm::vec3* radVertexColors);

    }
}
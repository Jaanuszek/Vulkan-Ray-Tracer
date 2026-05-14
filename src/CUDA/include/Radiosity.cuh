#pragma once

#include <cuda_runtime_api.h>
#include "CommonStructs.h"
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/sort.h>
#include <thrust/sequence.h>
#include <thrust/execution_policy.h>

// #include <cccl/thrust/host_vector.h>
// #include <cccl/thrust/device_vector.h>
// #include <cccl/thrust/sequence.h>
// #include <cccl/thrust/sort.h>
// #include <cccl/thrust/execution_policy.h>


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

        constexpr uint32_t SELECTED_PATCHES_COUNT = 2048;

        __global__ void filterPatches(Patch *patches, uint32_t numPatches,
                                      const SelectedPatch* alreadySelectedPatches,
                                      uint32_t alreadySelectedCount,
                                      SelectedPatch* selectedPatch);

        __global__ void reduceSelectedPatches(SelectedPatch *input, uint32_t n, SelectedPatch *output);

        __global__ void countSourceVisibilityHits(const PatchVisibility* visibilities, uint32_t numVisibilities,
                              const SelectedPatch* selectedPatch, uint32_t* sourceHitCounts);

        __global__ void countSourceVisibilityHits(const PatchVisibility* visibilities, uint32_t numVisibilities,
                              const uint32_t* selectedPatchIds, uint32_t* sourceHitCounts);

        __global__ void countSourceRaysShot(const PatchVisibility* visibilities, uint32_t numVisibilities,
                              const SelectedPatch* selectedPatches, uint32_t* totalRaysShot);

        __global__ void setEnergies(Patch *patches, uint32_t numPatches, float* energies);

        __global__ void calculateRadiosity(Patch *patches, uint32_t numPatches,
                                          PatchVisibility *visibilities, uint32_t numVisibilities,
                                          const SelectedPatch* selectedPatch,
                                          const uint32_t* sourceHitCounts,
                                          const uint32_t* totalRaysShot,
                                          float4* d_lightMap);              

        // Kernel odpowiadający za interpolacje kolorów wierzchołków
        // Bierze patche przypisane do danego wierzchołka i robi średnią ich kolorów
        __global__ void interpolateVertexColors(Patch* patches, uint32_t numPatches,
                                                const uint32_t* vertexPatchIndices, const uint32_t* vertexPatchOffsets,
                                                uint32_t numVertices, glm::vec3* radVertexColors);

        // Zamienia posortowane indeksy patchy na finalną tablicę SelectedPatch z id i energiami
        __global__ void writeTopKSelectedPatches(const Patch* patches,
                                             const uint32_t* sortedPatchIndices,
                                             const float* sortedEnergies,
                                             uint32_t numPatches,
                                             SelectedPatch* selectedPatch);

        __host__ void runFilterPatchesKernel(Patch* d_patches, uint32_t numPatches, SelectedPatch* d_selectedPatch, cudaStream_t stream);

        __host__ void runFilterPatchesKernelLegacy(Patch* d_patches, uint32_t numPatches, SelectedPatch* d_selectedPatch, cudaStream_t stream);

        __host__ void runFilterPatchesKernelTopK(Patch* d_patches, uint32_t numPatches, SelectedPatch* d_selectedPatch, cudaStream_t stream);

        __host__ void runPostVisibilityKernel(Patch* d_patches, uint32_t numPatches, 
                                                SelectedPatch* d_selectedPatch, PatchVisibility* d_visibilities, 
                                                uint32_t numVisibilities, float4* d_lightMap, cudaStream_t stream);

        __host__ void runInterpolateVertexKernel(Patch* d_patches, uint32_t numPatches,
                                            const uint32_t* d_vertexPatchIndices, const uint32_t* d_vertexPatchOffsets,
                                            uint32_t numVertices, glm::vec3* radVertexColors);

        __host__ void topKPatches(uint32_t numPatches, float* d_energies, uint32_t* d_selectedPatchesId, cudaStream_t stream);
    }
}
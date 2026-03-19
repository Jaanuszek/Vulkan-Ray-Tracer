#pragma once

#include <cuda_runtime_api.h>
#include "CommonStructs.h"


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
        /**
         * @brief Kernel do znalezienia patcha z największą niewystrzeloną energią
         * @param patches Array patchy
         * @param numPatches Ilość patchy
         * @param selectedPatch Output: wybrany patch (jeden thread zapisze wynik)
         */
        __global__ void filterPatches(Patch *patches, uint32_t numPatches,
                                      SelectedPatch* selectedPatch);

        __global__ void filterSelectedPatches(const SelectedPatch* inSelected,
                              SelectedPatch* outSelected,
                              uint32_t numSelected);

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
                                          uint32_t srcPatchId);
        
        __host__ void runFilterPatchesKernel(Patch* d_patches, uint32_t numPatches, SelectedPatch* d_selectedPatch, cudaStream_t stream);
    }
}
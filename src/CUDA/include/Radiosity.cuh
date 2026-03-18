#pragma once

#include <cuda_runtime_api.h>
#include "CommonStructs.h"

namespace VRTR
{
    namespace CUDA
    {
        /**
         * @brief Kernel do znalezienia patcha z największą niewystrzeloną energią
         * @param patches Array patchy
         * @param numPatches Ilość patchy
         * @param selectedPatch Output: wybrany patch (jeden thread zapisze wynik)
         */
        __global__ void filterPatches(Patch *patches, uint32_t numPatches,
                                      SelectedPatch* selectedPatch);

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
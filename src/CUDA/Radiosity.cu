#include "pch.h"
#include "Radiosity.cuh"

namespace VRTR::CUDA
{
    __global__ void postVisibilityKernelStub(Patch* patches, uint32_t numPatches, SelectedPatch* selectedPatch)
    {
        if (numPatches == 0 || selectedPatch == nullptr || patches == nullptr)
        {
            return;
        }

        // Placeholder stage for radiosity accumulation after visibility pass.
        // It currently performs no writes to keep behavior deterministic while integrating pipeline flow.
    }

    __global__ void filterPatches(Patch* patches, uint32_t numPatches,
                                  SelectedPatch* selectedPatch)
    {
        uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;

        __shared__ float maxEnergy[TPB];
        __shared__ uint32_t maxPatchId[TPB];

        float localMaxEnergy = 0.0f;
        uint32_t localMaxPatchId = 0;

        for (uint32_t i = idx; i < numPatches; i += gridDim.x * blockDim.x)
        {
            if (patches[i].unshotEnergy > localMaxEnergy)
            {
                localMaxEnergy = patches[i].unshotEnergy;
                localMaxPatchId = patches[i].id;
            }
        }

        maxEnergy[threadIdx.x] = localMaxEnergy;
        maxPatchId[threadIdx.x] = localMaxPatchId;
        __syncthreads(); // obowiązkowo synchronizacja zeby wszystkie wątki zdążyły zapisać

        for (uint32_t s = blockDim.x / 2; s > 0; s >>= 1)
        {
            if(threadIdx.x < s)
            {
                if (maxEnergy[threadIdx.x + s] > maxEnergy[threadIdx.x])
                {
                    maxEnergy[threadIdx.x] = maxEnergy[threadIdx.x + s];
                    maxPatchId[threadIdx.x] = maxPatchId[threadIdx.x + s];
                }
            }
            __syncthreads(); 
        }

        if (threadIdx.x == 0)
        {
            // Jeden wynik na blok: zapis do indeksu blockIdx.x.
            selectedPatch[blockIdx.x].patchId = maxPatchId[0];
            selectedPatch[blockIdx.x].unshotEnergy = maxEnergy[0];
        }
    }

    __global__ void reduceSelectedPatches(SelectedPatch* input, uint32_t n, SelectedPatch* output)
    {
        __shared__ float maxEnergy[TPB];
        __shared__ uint32_t maxPatchId[TPB];

        uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;

        float localMax = -FLT_MAX;
        uint32_t localId = 0;
        for (uint32_t i = idx; i < n; i += blockDim.x * gridDim.x)
        {
            if (input[i].unshotEnergy > localMax)
            {
                localMax = input[i].unshotEnergy;
                localId = input[i].patchId;
            }
        }

        maxEnergy[threadIdx.x] = localMax;
        maxPatchId[threadIdx.x] = localId;
        __syncthreads();

        for(uint32_t s = blockDim.x / 2; s > 0; s >>= 1)
        {
            if(threadIdx.x < s)
            {
                if (maxEnergy[threadIdx.x + s] > maxEnergy[threadIdx.x])
                {
                    maxEnergy[threadIdx.x] = maxEnergy[threadIdx.x + s];
                    maxPatchId[threadIdx.x] = maxPatchId[threadIdx.x + s];
                }
            }
            __syncthreads(); 
        }

        if(threadIdx.x == 0)
        {
            output[blockIdx.x].patchId = maxPatchId[0];
            output[blockIdx.x].unshotEnergy = maxEnergy[0];
        }
    }

    __global__ void calculateRadiosity()
    {
        uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
        // Placeholder for radiosity calculation kernel.
    }

    // /**
    //  * KERNEL 2: calculateRadiosity - obliczenie transferu radiosity między patchami
    //  * 
    //  * Na bazie visibility z Vulkan ray tracingu, obliczamy ile energii
    //  * transfer się z srcPatch do pozostałych patchy
    //  */
    // __global__ void calculateRadiosity(Patch *patches, uint32_t numPatches,
    //                                    PatchVisibility *visibilities, uint32_t numVisibilities,
    //                                    uint32_t srcPatchId)
    // {
    //     uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
        
    //     if (idx >= numVisibilities)
    //         return;
        
    //     PatchVisibility &vis = visibilities[idx];
        
    //     // Jeśli to nie jest nasze source patch, skip
    //     if (vis.srcPatchId != srcPatchId)
    //         return;
        
    //     Patch &srcPatch = patches[srcPatchId];
    //     Patch &dstPatch = patches[vis.dstPatchId];
        
    //     // Jeśli destination patch nie jest widoczny, skip
    //     if (vis.visibility < 0.001f)
    //         return;
        
    //     // Obliczamy form factor F (solid angle approximation)
    //     glm::vec3 diff = dstPatch.center - srcPatch.center;
    //     float distance = glm::length(diff);
        
    //     if (distance < 0.001f)
    //         return; // Zbyt blisko, ignoruj
        
    //     glm::vec3 dir = diff / distance;
        
    //     // Cosine term z obu stron
    //     float cosSrc = glm::max(0.0f, glm::dot(srcPatch.normal, dir));
    //     float cosDst = glm::max(0.0f, glm::dot(dstPatch.normal, -dir));
        
    //     // Form factor (uproszczona wersja)
    //     float distSq = distance * distance;
    //     float formFactor = (cosSrc * cosDst) / (M_PI * distSq);
    //     formFactor *= dstPatch.area;
        
    //     // Radiosity transfer z visibility
    //     float transferEnergy = srcPatch.unshotEnergy * formFactor * vis.visibility;
        
    //     // Albedo modulates transfer
    //     transferEnergy *= glm::length(dstPatch.albedo) / 3.0f;  // średnia albedo
        
    //     // Aktualizuj radiosity destination patcha
    //     // Note: w idealnym case miałbyś reduction/atomic, ale dla prostoty używamy atomic
    //     atomicAdd(&dstPatch.radiosity, transferEnergy);
    // }
    __host__ void runFilterPatchesKernel(Patch *patches, uint32_t numPatches, SelectedPatch* selectedPatch, cudaStream_t stream)
    {
        if (numPatches == 0)
            return;

        int blocks = (numPatches + TPB - 1) / TPB;
        blocks = min(blocks, 1024);

        SelectedPatch* d_blockResults = nullptr;
        CUDA_CHECK_STD_ERROR(cudaMalloc(&d_blockResults, blocks * sizeof(SelectedPatch)));

        filterPatches<<<blocks, TPB, 0, stream>>>(
            patches,
            numPatches,
            d_blockResults // W przypadku gdy blockow jest wiecej niz 1, czyli jak mamy tablice wieksza niz 1024, to to jest tablicą o rozmiarze blocks
        );
        CUDA_CHECK_STD_ERROR(cudaGetLastError());

        uint32_t currSize = blocks;

        SelectedPatch* d_reduceBuffer = nullptr;
        if (currSize > 1)
        {
            CUDA_CHECK_STD_ERROR(cudaMalloc(&d_reduceBuffer, blocks * sizeof(SelectedPatch)));
        }

        SelectedPatch* d_in = d_blockResults;
        SelectedPatch* d_out = d_reduceBuffer;

        while(currSize > 1)
        {
            uint32_t nextSize = (currSize + TPB - 1) / TPB;

            reduceSelectedPatches<<<nextSize, TPB, 0, stream>>>(
                d_in,
                currSize,
                d_out
            );
            CUDA_CHECK_STD_ERROR(cudaGetLastError());

            std::swap(d_in, d_out);

            currSize = nextSize;
        }

        CUDA_CHECK_STD_ERROR(cudaMemcpyAsync(
            selectedPatch,
            d_in,
            sizeof(SelectedPatch),
            cudaMemcpyDeviceToDevice,
            stream
        ));

        cudaFree(d_blockResults);
        if (d_reduceBuffer)
        {
            cudaFree(d_reduceBuffer);
        }
    }

    // __host__ void runPostVisibilityKernelStub(Patch* patches, uint32_t numPatches, SelectedPatch* selectedPatch, cudaStream_t stream)
    // {
    //     postVisibilityKernelStub<<<1, 1, 0, stream>>>(patches, numPatches, selectedPatch);
    //     CUDA_CHECK_STD_ERROR(cudaGetLastError());
    // }

    __host__ void runPostVisibilityKernel(cudaStream_t stream)
    {
        calculateRadiosity<<<1, 1, 0, stream>>>();
        CUDA_CHECK_STD_ERROR(cudaGetLastError());
    }
}
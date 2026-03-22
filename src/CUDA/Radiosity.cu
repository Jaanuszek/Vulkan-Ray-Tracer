#include "pch.h"
#include "Radiosity.cuh"

namespace VRTR::CUDA
{
    namespace
    {
        constexpr float CONVERGENCE_UNSHOT_EPSILON = 1e-5f;
    }

    __device__ inline float3 makeDiff(const glm::vec3& a, const glm::vec3& b)
    {
        return make_float3(a.x - b.x, a.y - b.y, a.z - b.z);
    }

    __device__ inline float dot3(const float3& a, const float3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    __device__ inline float length3(const float3& v)
    {
        return sqrtf(dot3(v, v));
    }

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
    // /**
    //  * KERNEL 2: calculateRadiosity - obliczenie transferu radiosity między patchami
    //  * 
    //  * Na bazie visibility z Vulkan ray tracingu, obliczamy ile energii
    //  * transfer się z srcPatch do pozostałych patchy
    //  */
    __global__ void calculateRadiosity(Patch *patches, uint32_t numPatches,
                                       PatchVisibility *visibilities, uint32_t numVisibilities,
                                       SelectedPatch* selectedPatch, float4* d_lightMap)
    {
        uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
        
        if (idx >= numVisibilities)
            return;
        
        PatchVisibility &vis = visibilities[idx];
        
        // Jeśli to nie jest nasze source patch, skip
        if (vis.srcPatchId != selectedPatch->patchId)
            return;
        
        Patch &srcPatch = patches[selectedPatch->patchId];
        Patch &dstPatch = patches[vis.dstPatchId];
        
        // Jeśli destination patch nie jest widoczny, skip
        if (vis.visibility < 0.001f || dstPatch.id == 0xFFFFFFFF)
            return;

    
        // numVisibilities to liczba promieni wystrzelanych z patchy
        // narazie robie to w jednym wymiarze
        // ale trzeba bedzie to zmienic na 3d
        float energyPerRay = srcPatch.unshotEnergy / numVisibilities;

        // Obliczamy form factor F (solid angle approximation)
        glm::vec3 diff = dstPatch.center - srcPatch.center;
        float distance = glm::length(diff);
        
        if (distance < 0.001f)
            return; // Zbyt blisko, ignoruj
        
        glm::vec3 dir = diff / distance;
        
        // Cosine term z obu stron
        float cosSrc = glm::max(0.0f, glm::dot(srcPatch.normal, dir));
        float cosDst = glm::max(0.0f, glm::dot(dstPatch.normal, -dir));
        
        // Form factor (uproszczona wersja)
        float distSq = distance * distance;
        float formFactor = (cosSrc * cosDst) / (M_PI * distSq);
        formFactor *= dstPatch.area;
        formFactor = min(1.0f, formFactor);

        // Radiosity transfer z visibility
        // float transferEnergy = srcPatch.unshotEnergy * formFactor * vis.visibility;
        // glm::vec3 transferEnergy = srcPatch.unshotEnergy * formFactor * vis.visibility * dstPatch.albedo;
        glm::vec3 transferEnergy = energyPerRay * dstPatch.albedo;
        // Aktualizuj radiosity destination patcha
        // Note: w idealnym case miałbyś reduction/atomic, ale dla prostoty używamy atomic
        atomicAdd(&dstPatch.radiosity.r, transferEnergy.r);
        atomicAdd(&dstPatch.radiosity.g, transferEnergy.g);
        atomicAdd(&dstPatch.radiosity.b, transferEnergy.b);
        dstPatch.unshotEnergy += energyPerRay;
        // atomicAdd(&dstPatch.unshotEnergy, 0.2f);
        // dstPatch.unshotEnergy += transferEnergy.r + transferEnergy.g + transferEnergy.b;

        d_lightMap[dstPatch.id] = make_float4(transferEnergy.r, transferEnergy.g, transferEnergy.b, 1.0f);
    }

    __global__ void resetSelectedPatchUnshotEnergy(Patch* patches, uint32_t numPatches, SelectedPatch* selectedPatch)
    {
        if (patches == nullptr || selectedPatch == nullptr)
        {
            return;
        }

        if (blockIdx.x == 0 && threadIdx.x == 0)
        {
            const uint32_t selectedPatchId = selectedPatch->patchId;
            if (selectedPatchId < numPatches)
            {
                patches[selectedPatchId].unshotEnergy = 0.0f;
                selectedPatch->unshotEnergy = 0.0f;
            }
        }
    }

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

    __host__ void runPostVisibilityKernel(Patch* d_patches, uint32_t numPatches, 
                                        SelectedPatch* d_selectedPatch, PatchVisibility* d_visibilities, 
                                        uint32_t numVisibilities, float4* d_lightMap, cudaStream_t stream)
    {
        if (numPatches == 0)
        {
            return;
        }

        int blocks = (numPatches + TPB - 1) / TPB;
        blocks = min(blocks, 1024);

        calculateRadiosity<<<blocks, TPB, 0, stream>>>(d_patches, numPatches, d_visibilities, numVisibilities, d_selectedPatch, d_lightMap);
        CUDA_CHECK_STD_ERROR(cudaGetLastError());

        // Reset source patch energy after radiosity accumulation so the next selection pass sees the update.
        resetSelectedPatchUnshotEnergy<<<1, 1, 0, stream>>>(d_patches, numPatches, d_selectedPatch);
        CUDA_CHECK_STD_ERROR(cudaGetLastError());
    }
}
#include "pch.h"
#include "Radiosity.cuh"

namespace VRTR::CUDA
{
    namespace
    {
        __host__ __device__ inline float energyMetric(const glm::vec3& e)
        {
            return (e.r + e.g + e.b) / 3.0f;
        }
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

    __device__ inline bool isAlreadySelected(const SelectedPatch* selectedPatches,
                                             uint32_t alreadySelectedCount,
                                             uint32_t patchId)
    {
        for (uint32_t i = 0; i < alreadySelectedCount; ++i)
        {
            if (selectedPatches[i].patchId == patchId)
            {
                return true;
            }
        }

        return false;
    }

    __global__ void filterPatches(Patch* patches, uint32_t numPatches,
                                  const SelectedPatch* alreadySelectedPatches,
                                  uint32_t alreadySelectedCount,
                                  SelectedPatch* selectedPatch)
    {
        uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;

        __shared__ float maxEnergy[TPB];
        __shared__ uint32_t maxPatchId[TPB];

        float localMaxEnergy = -FLT_MAX;
        uint32_t localMaxPatchId = 0xFFFFFFFF;

        // grid-stride loop
        for (uint32_t i = idx; i < numPatches; i += gridDim.x * blockDim.x)
        {
            if (isAlreadySelected(alreadySelectedPatches, alreadySelectedCount, patches[i].id))
            {
                continue;
            }

            const float patchEnergy = energyMetric(patches[i].unshotEnergy);
            if (patchEnergy > localMaxEnergy)
            {
                localMaxEnergy = patchEnergy;
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
            selectedPatch[blockIdx.x].patchId = maxPatchId[0];
            selectedPatch[blockIdx.x].unshotEnergy = (maxEnergy[0] > 0.0f) ? maxEnergy[0] : 0.0f;
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

    __global__ void countSourceVisibilityHits(const PatchVisibility* visibilities,
                                              uint32_t numVisibilities,
                                              const SelectedPatch* selectedPatch,
                                              uint32_t* sourceHitCounts)
    {
        uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
        if (idx >= numVisibilities)
        {
            return;
        }

        const PatchVisibility& vis = visibilities[idx];
        if (vis.srcPatchId == 0xFFFFFFFF || vis.dstPatchId == 0xFFFFFFFF || vis.visibility < 0.001f)
        {
            return;
        }

        for (uint32_t selectedIdx = 0; selectedIdx < SELECTED_PATCHES_COUNT; ++selectedIdx)
        {
            if (selectedPatch[selectedIdx].patchId == vis.srcPatchId)
            {
                atomicAdd(&sourceHitCounts[selectedIdx], 1u);
                break;
            }
        }
    }

    __global__ void calculateRadiosity(Patch *patches, uint32_t numPatches,
                                       PatchVisibility *visibilities, uint32_t numVisibilities,
                                       const SelectedPatch* selectedPatch,
                                       const uint32_t* sourceHitCounts,
                                       float4* d_lightMap)
    {
        uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
        
        if (idx >= numVisibilities)
            return;
        
        PatchVisibility &vis = visibilities[idx];

        if (vis.srcPatchId >= numPatches || vis.dstPatchId >= numPatches)
            return;
        
        Patch &dstPatch = patches[vis.dstPatchId];
        
        // Jeśli destination patch nie jest widoczny, skip
        if (vis.visibility < 0.001f || dstPatch.id == 0xFFFFFFFF || dstPatch.id >= numPatches)
            return;

        const uint32_t sourcePatchId = vis.srcPatchId;

        if (sourcePatchId == 0xFFFFFFFF || sourcePatchId >= numPatches)
        {
            return;
        }

        if (vis.dstPatchId == sourcePatchId)
        {
            return;
        }

        uint32_t selectedSourceIdx = 0xFFFFFFFF;
        for (uint32_t selectedIdx = 0; selectedIdx < SELECTED_PATCHES_COUNT; ++selectedIdx)
        {
            if (selectedPatch[selectedIdx].patchId == sourcePatchId)
            {
                selectedSourceIdx = selectedIdx;
                break;
            }
        }

        if (selectedSourceIdx == 0xFFFFFFFF)
        {
            return;
        }

        Patch &srcPatch = patches[sourcePatchId];

        const uint32_t validHitCount = max(sourceHitCounts[selectedSourceIdx], 1u);
        glm::vec3 energyPerRay = (srcPatch.unshotEnergy) / static_cast<float>(validHitCount);

        const glm::vec3 transferEnergy = energyPerRay * vis.visibility * dstPatch.albedo;

        // Aktualizuj radiosity destination patcha.
        atomicAdd(&dstPatch.radiosity.r, transferEnergy.r);
        atomicAdd(&dstPatch.radiosity.g, transferEnergy.g);
        atomicAdd(&dstPatch.radiosity.b, transferEnergy.b);
        atomicAdd(&dstPatch.unshotEnergy.r, transferEnergy.r);
        atomicAdd(&dstPatch.unshotEnergy.g, transferEnergy.g);
        atomicAdd(&dstPatch.unshotEnergy.b, transferEnergy.b);

        atomicAdd(&d_lightMap[dstPatch.id].x, transferEnergy.r);
        atomicAdd(&d_lightMap[dstPatch.id].y, transferEnergy.g);
        atomicAdd(&d_lightMap[dstPatch.id].z, transferEnergy.b);
        atomicExch(&d_lightMap[dstPatch.id].w, 1.0f);
    }

    __global__ void interpolateVertexColors(Patch* patches, uint32_t numPatches,
                                            const uint32_t* vertexPatchIndices, const uint32_t* vertexPatchOffsets,
                                            uint32_t numVertices)
    {
        uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;

        if (idx >= numVertices) return;

        for (uint32_t i = idx; i < numVertices; i += blockDim.x * gridDim.x)
        {
            glm::vec3 color(0.0f);
            float totalArea = 0.0f;

            uint32_t patchStart = vertexPatchOffsets[i];
            uint32_t patchEnd = vertexPatchOffsets[i + 1];

            for (uint32_t j = patchStart; j < patchEnd; ++j)
            {
                uint32_t patchId = vertexPatchIndices[j];
                if (patchId < numPatches)
                {
                    float area = patches[patchId].area;
                    color += patches[patchId].radiosity * area;
                    totalArea += area;
                }
            }

            // atomicAdd(&d_lightMap[i].x, (totalArea > 0.0f) ? (color.r / totalArea) : 0.0f);
            // atomicAdd(&d_lightMap[i].y, (totalArea > 0.0f) ? (color.g / totalArea) : 0.0f);
            // atomicAdd(&d_lightMap[i].z, (totalArea > 0.0f) ? (color.b / totalArea) : 0.0f);
        }
    }

    __global__ void resetSelectedPatchUnshotEnergy(Patch* patches, uint32_t numPatches, SelectedPatch* selectedPatch)
    {
        if (patches == nullptr || selectedPatch == nullptr)
        {
            return;
        }

        const uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
        if (idx >= SELECTED_PATCHES_COUNT)
        {
            return;
        }

        const uint32_t selectedPatchId = selectedPatch[idx].patchId;
        if (selectedPatchId != 0xFFFFFFFF)
        {
            if (selectedPatchId < numPatches)
            {
                patches[selectedPatchId].unshotEnergy = glm::vec3(0.0f);
            }

            selectedPatch[idx].patchId = 0xFFFFFFFF;
            selectedPatch[idx].unshotEnergy = 0.0f;
        }
    }

    __host__ void runFilterPatchesKernel(Patch *patches, uint32_t numPatches, SelectedPatch* selectedPatch, cudaStream_t stream)
    {
        if (numPatches == 0)
            return;

        int blocks = (numPatches + TPB - 1) / TPB;
        blocks = min(blocks, 1024);

        SelectedPatch initialSelection[SELECTED_PATCHES_COUNT]{};
        for (uint32_t i = 0; i < SELECTED_PATCHES_COUNT; ++i)
        {
            initialSelection[i].patchId = 0xFFFFFFFF;
            initialSelection[i].unshotEnergy = 0.0f;
        }

        CUDA_CHECK_STD_ERROR(cudaMemcpyAsync(
            selectedPatch,
            initialSelection,
            sizeof(initialSelection),
            cudaMemcpyHostToDevice,
            stream
        ));

        SelectedPatch* d_blockResults = nullptr;
        CUDA_CHECK_STD_ERROR(cudaMalloc(&d_blockResults, blocks * sizeof(SelectedPatch)));

        SelectedPatch* d_reduceBuffer = nullptr;
        if (blocks > 1)
        {
            CUDA_CHECK_STD_ERROR(cudaMalloc(&d_reduceBuffer, blocks * sizeof(SelectedPatch)));
        }

        for (uint32_t selectedIdx = 0; selectedIdx < SELECTED_PATCHES_COUNT; ++selectedIdx)
        {
            filterPatches<<<blocks, TPB, 0, stream>>>(
                patches,
                numPatches,
                selectedPatch,
                selectedIdx,
                d_blockResults
            );
            CUDA_CHECK_STD_ERROR(cudaGetLastError());

            uint32_t currSize = blocks;
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
                selectedPatch + selectedIdx,
                d_in,
                sizeof(SelectedPatch),
                cudaMemcpyDeviceToDevice,
                stream
            ));
        }

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
        if (numPatches == 0 || numVisibilities == 0)
        {
            return;
        }

        int blocks = (numVisibilities + TPB - 1) / TPB;
        blocks = min(blocks, 1024);

        uint32_t* d_sourceHitCounts = nullptr;
        CUDA_CHECK_STD_ERROR(cudaMalloc(&d_sourceHitCounts, sizeof(uint32_t) * SELECTED_PATCHES_COUNT));
        CUDA_CHECK_STD_ERROR(cudaMemsetAsync(d_sourceHitCounts, 0, sizeof(uint32_t) * SELECTED_PATCHES_COUNT, stream));

        countSourceVisibilityHits<<<blocks, TPB, 0, stream>>>(
            d_visibilities,
            numVisibilities,
            d_selectedPatch,
            d_sourceHitCounts
        );
        CUDA_CHECK_STD_ERROR(cudaGetLastError());

        calculateRadiosity<<<blocks, TPB, 0, stream>>>(
            d_patches,
            numPatches,
            d_visibilities,
            numVisibilities,
            d_selectedPatch,
            d_sourceHitCounts,
            d_lightMap
        );
        CUDA_CHECK_STD_ERROR(cudaGetLastError());

        // Reset source patch energy after radiosity accumulation so the next selection pass sees the update.
        const uint32_t resetThreads = min(TPB, SELECTED_PATCHES_COUNT);
        const uint32_t resetBlocks = (SELECTED_PATCHES_COUNT + resetThreads - 1) / resetThreads;
        resetSelectedPatchUnshotEnergy<<<resetBlocks, resetThreads, 0, stream>>>(d_patches, numPatches, d_selectedPatch);
        CUDA_CHECK_STD_ERROR(cudaGetLastError());

        cudaFree(d_sourceHitCounts);
    }
}
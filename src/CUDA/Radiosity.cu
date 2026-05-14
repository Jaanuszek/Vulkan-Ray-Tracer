#include "pch.h"
#include "Radiosity.cuh"

namespace VRTR::CUDA
{
    namespace
    {
        __host__ __device__ inline float energyMetric(const glm::vec3& e)
        {
            // return (e.r + e.g + e.b) / 3.0f;
            return 0.2126f * e.r + 0.7152f * e.g + 0.0722f * e.b; // luminance
        }
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
            selectedPatch[blockIdx.x].totalRaysShot = 0u;  // Zostanie zmienione w countSourceRaysShot
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
            output[blockIdx.x].totalRaysShot = 0u;  // Zostanie zmienione w countSourceRaysShot
        }
    }

    __global__ void setEnergies(Patch *patches, uint32_t numPatches, float* energies)
    {
        uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
        if(idx >= numPatches)
            return;
        const glm::vec3 energy = patches[idx].unshotEnergy;
        energies[idx] = energyMetric(energy);
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
        if (vis.srcPatchId == 0xFFFFFFFF || vis.dstPatchId == 0xFFFFFFFF || vis.visibility == 0.0f)
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

    __global__ void countSourceVisibilityHits(const PatchVisibility* visibilities,
                                              uint32_t numVisibilities,
                                              const uint32_t* selectedPatchIds,
                                              uint32_t* sourceHitCounts)
    {
        uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
        if (idx >= numVisibilities)
        {
            return;
        }

        const PatchVisibility& vis = visibilities[idx];
        if (vis.srcPatchId == 0xFFFFFFFF || vis.dstPatchId == 0xFFFFFFFF || vis.visibility == 0.0f)
        {
            return;
        }

        for (uint32_t selectedIdx = 0; selectedIdx < SELECTED_PATCHES_COUNT; ++selectedIdx)
        {
            if (selectedPatchIds[selectedIdx] == vis.srcPatchId)
            {
                atomicAdd(&sourceHitCounts[selectedIdx], 1u);
                break;
            }
        }
    }

    // Liczy CAŁKOWITĄ liczbę promieni wystrzelonych z każdej wybranej patchy
    // (zarówno te, które trafiły, jak i te, które nie trafiły)
    __global__ void countSourceRaysShot(const PatchVisibility* visibilities,
                                        uint32_t numVisibilities,
                                        const SelectedPatch* selectedPatches,
                                        uint32_t* totalRaysShot)
    {
        uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
        if (idx >= numVisibilities)
        {
            return;
        }

        const PatchVisibility& vis = visibilities[idx];
        if (vis.srcPatchId == 0xFFFFFFFF)
        {
            return;
        }

        for (uint32_t selectedIdx = 0; selectedIdx < SELECTED_PATCHES_COUNT; ++selectedIdx)
        {
            if (selectedPatches[selectedIdx].patchId == vis.srcPatchId)
            {
                atomicAdd(&totalRaysShot[selectedIdx], 1u);
                break;
            }
        }
    }

    __global__ void calculateRadiosity(Patch *patches, uint32_t numPatches,
                                       PatchVisibility *visibilities, uint32_t numVisibilities,
                                       const SelectedPatch* selectedPatch,
                                       const uint32_t* sourceHitCounts,
                                       const uint32_t* totalRaysShot,
                                       float4* d_lightMap)
    {
        uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
        
        if (idx >= numVisibilities)
            return;
        
        PatchVisibility &vis = visibilities[idx];

        if (vis.srcPatchId >= numPatches || vis.dstPatchId >= numPatches)
            return;
        
        Patch &dstPatch = patches[vis.dstPatchId];
        Patch &srcPatch = patches[vis.srcPatchId];
        
        // Jeśli destination patch nie jest widoczny, skip
        if (vis.visibility == 0.0f || dstPatch.id == 0xFFFFFFFF || dstPatch.id >= numPatches)
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

        const uint32_t totalRays = totalRaysShot[selectedSourceIdx];
        if (totalRays == 0)
        {
            return; // Nie promienie wystrzelone, skip
        }
        glm::vec3 energyPerRay = (srcPatch.unshotEnergy) / static_cast<float>(totalRays);
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
                                            uint32_t numVertices, glm::vec3* radVertexColors)
    {
        uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;

        if (idx >= numVertices) return;

        for (uint32_t i = idx; i < numVertices; i += blockDim.x * gridDim.x)
        {
            glm::vec3 color(0.0f);
            float totalWeight = 0.0f;

            uint32_t patchStart = vertexPatchOffsets[i];
            uint32_t patchEnd = vertexPatchOffsets[i + 1];
            
            uint32_t patchCount = patchEnd - patchStart;
            if (patchCount == 0) continue;

            // Najpierw wylicz centroid wierzchołka (średnia z center patchy)
            glm::vec3 vertexEstimate(0.0f);
            float avgArea = 0.0f;
            float maxDist = 0.0f;
            
            for (uint32_t j = patchStart; j < patchEnd; ++j)
            {
                uint32_t patchId = vertexPatchIndices[j];
                if (patchId < numPatches)
                {
                    vertexEstimate += patches[patchId].center;
                    avgArea += patches[patchId].area;
                }
            }
            vertexEstimate /= static_cast<float>(patchCount);
            avgArea /= static_cast<float>(patchCount);
            avgArea = glm::max(0.001f, avgArea);

            // Precompute max distance dla normalizacji distance-based weighting
            for (uint32_t j = patchStart; j < patchEnd; ++j)
            {
                uint32_t patchId = vertexPatchIndices[j];
                if (patchId < numPatches)
                {
                    glm::vec3 toVertex = vertexEstimate - patches[patchId].center;
                    float dist = glm::length(toVertex);
                    maxDist = glm::max(maxDist, dist);
                }
            }
            maxDist = glm::max(0.001f, maxDist);

            // Interpolacja z ulepszonymi wagami: znormalizowana area + distance + normal orientation
            for (uint32_t j = patchStart; j < patchEnd; ++j)
            {
                uint32_t patchId = vertexPatchIndices[j];
                if (patchId < numPatches)
                {
                    float patchArea = patches[patchId].area;
                    
                    // 1. Area weight - znormalizowany względem średniej (unika dominacji dużych patchy)
                    float areaWeight = patchArea / avgArea;
                    areaWeight = glm::pow(areaWeight, 0.5f); // Łagodne skalowanie - unika ekstremalnych różnic
                    areaWeight = glm::clamp(areaWeight, 0.2f, 5.0f); // Clamp extreme values [0.2, 5.0]

                    // 2. Distance weighting - bliskie patchy mają wyższą wagę
                    glm::vec3 toVertex = vertexEstimate - patches[patchId].center;
                    float dist = glm::length(toVertex);
                    float distWeight = 1.0f - (dist / maxDist) * 0.7f; // Falloff od 0.3 do 1.0
                    distWeight = glm::max(0.3f, distWeight);

                    // 3. Normal weighting - patchy skierowane w stronę vertex mają wyższą wagę
                    float normalWeight = 1.0f;
                    if (dist > 0.001f)
                    {
                        // Cosine weighting - patchy z lepszą orientacją mają więcej wpływu
                        normalWeight = 0.6f + 0.4f * glm::dot(patches[patchId].normal, glm::normalize(toVertex));
                        normalWeight = glm::max(0.2f, normalWeight);
                    }

                    // Combined weight - multiplikatywnie aby zachować naturalną interpolację
                    float weight = areaWeight * distWeight * normalWeight;
                    color += patches[patchId].radiosity * weight;
                    totalWeight += weight;
                }
            }

            if (totalWeight > 0.0f)
            {
                radVertexColors[i] = color / totalWeight;
            }
            else
            {
                radVertexColors[i] = glm::vec3(0.0f);
            }
        }
    }

    __global__ void resetSelectedPatchUnshotEnergy(Patch* patches, uint32_t numPatches, SelectedPatch* selectedPatch, const uint32_t* sourceHitCounts)
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
            // const uint32_t validHitCount = sourceHitCounts[selectedPatch[idx].patchId];
            // float shotFraction = (validHitCount > 0) ? (1.0f / static_cast<float>(validHitCount)) : 0.0f;

            selectedPatch[idx].patchId = 0xFFFFFFFF;

            selectedPatch[idx].unshotEnergy = 0.0f;
            selectedPatch[idx].totalRaysShot = 0u;
        }
    }

    __global__ void writeTopKSelectedPatches(const Patch* patches,
                                             const uint32_t* sortedPatchIndices,
                                             const float* sortedEnergies,
                                             uint32_t numPatches,
                                             SelectedPatch* selectedPatch)
    {
        const uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
        if (idx >= SELECTED_PATCHES_COUNT)
        {
            return;
        }

        if (idx < numPatches)
        {
            const uint32_t patchIdx = sortedPatchIndices[idx];
            // Czy patchIdx to nie bedzie to samo co patches[patchIdx].id?
            selectedPatch[idx].patchId = patches[patchIdx].id;
            selectedPatch[idx].unshotEnergy = max(sortedEnergies[idx], 0.0f);
            selectedPatch[idx].totalRaysShot = 0u;  // Zostanie zmienione w countSourceRaysShot
            return;
        }

        selectedPatch[idx].patchId = 0xFFFFFFFF;
        selectedPatch[idx].unshotEnergy = 0.0f;
        selectedPatch[idx].totalRaysShot = 0u;
    }

    __host__ void runFilterPatchesKernelLegacy(Patch *patches, uint32_t numPatches, SelectedPatch* selectedPatch, cudaStream_t stream)
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
            initialSelection[i].totalRaysShot = 0u;
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
            cudaEvent_t start, stop;
            float elapsedTime = 0.0f;
            cudaEventCreate(&start);
            cudaEventCreate(&stop);
            cudaEventRecord(start, stream);

            filterPatches<<<blocks, TPB, 0, stream>>>(
                patches,
                numPatches,
                selectedPatch,
                selectedIdx,
                d_blockResults);
            CUDA_CHECK_STD_ERROR(cudaGetLastError());

            cudaEventRecord(stop, stream);
            cudaEventSynchronize(stop);
            cudaEventElapsedTime(&elapsedTime, start, stop);
            // std::cout << "FilterPatches legacy kernel for selected patch " << selectedIdx << " took " << elapsedTime << " ms\n";
            cudaEventDestroy(start);
            cudaEventDestroy(stop);

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

    __host__ void runFilterPatchesKernelTopK(Patch *patches, uint32_t numPatches, SelectedPatch* selectedPatch, cudaStream_t stream)
    {
        if (numPatches == 0)
            return;

        cudaEvent_t start, stop;
        float elapsedTime = 0.0f;
        cudaEventCreate(&start);
        cudaEventCreate(&stop);
        cudaEventRecord(start, stream);

        float* d_energies = nullptr;
        CUDA_CHECK_STD_ERROR(cudaMalloc(&d_energies, sizeof(float) * numPatches));
        // chyba git?
        CUDA_CHECK_STD_ERROR(cudaMemsetAsync(d_energies, 0xFFFFFFFF, sizeof(float) * numPatches, stream));

        uint32_t* d_topPatchIndices = nullptr;
        CUDA_CHECK_STD_ERROR(cudaMalloc(&d_topPatchIndices, sizeof(uint32_t) * SELECTED_PATCHES_COUNT));

        const uint32_t energyBlocks = min((numPatches + TPB - 1) / TPB, 1024u);
        setEnergies<<<energyBlocks, TPB, 0, stream>>>(patches, numPatches, d_energies);
        CUDA_CHECK_STD_ERROR(cudaGetLastError());

        topKPatches(numPatches, d_energies, d_topPatchIndices, stream);

        const uint32_t writeBlocks = (SELECTED_PATCHES_COUNT + TPB - 1) / TPB;
        const uint32_t topCount = min(numPatches, SELECTED_PATCHES_COUNT);
        writeTopKSelectedPatches<<<writeBlocks, TPB, 0, stream>>>(
            patches,
            d_topPatchIndices,
            d_energies,
            topCount,
            selectedPatch
        );
        CUDA_CHECK_STD_ERROR(cudaGetLastError());

        cudaEventRecord(stop, stream);
        cudaEventSynchronize(stop);
        cudaEventElapsedTime(&elapsedTime, start, stop);
        // std::cout << "FilterPatches top-k total took " << elapsedTime << " ms\n";
        cudaEventDestroy(start);
        cudaEventDestroy(stop);

        cudaFree(d_topPatchIndices);
        cudaFree(d_energies);
    }

    __host__ void runFilterPatchesKernel(Patch *patches, uint32_t numPatches, SelectedPatch* selectedPatch, cudaStream_t stream)
    {
        runFilterPatchesKernelTopK(patches, numPatches, selectedPatch, stream);
        // runFilterPatchesKernelLegacy(patches, numPatches, selectedPatch, stream);
    }

    __host__ void runPostVisibilityKernel(Patch* d_patches, uint32_t numPatches, 
                                        SelectedPatch* d_selectedPatch, PatchVisibility* d_visibilities, 
                                        uint32_t numVisibilities, float4* d_lightMap, cudaStream_t stream)
    {
        if (numPatches == 0 || numVisibilities == 0)
        {
            return;
        }

        cudaEvent_t start, stop;
        float elapsedTime = 0.0f;
        cudaEventCreate(&start);
        cudaEventCreate(&stop);

        int blocks = (numVisibilities + TPB - 1) / TPB;
        blocks = min(blocks, 1024);

        uint32_t* d_sourceHitCounts = nullptr;
        CUDA_CHECK_STD_ERROR(cudaMalloc(&d_sourceHitCounts, sizeof(uint32_t) * SELECTED_PATCHES_COUNT));
        CUDA_CHECK_STD_ERROR(cudaMemsetAsync(d_sourceHitCounts, 0, sizeof(uint32_t) * SELECTED_PATCHES_COUNT, stream));

        uint32_t* d_totalRaysShot = nullptr;
        CUDA_CHECK_STD_ERROR(cudaMalloc(&d_totalRaysShot, sizeof(uint32_t) * SELECTED_PATCHES_COUNT));
        CUDA_CHECK_STD_ERROR(cudaMemsetAsync(d_totalRaysShot, 0, sizeof(uint32_t) * SELECTED_PATCHES_COUNT, stream));

        // Dla countSourceRaysShot potrzebujemy więcej bloków, aby pokryć wszystkie visibility records
        int blocksForRaysShot = (numVisibilities + TPB - 1) / TPB;

        cudaEventRecord(start, stream);
        countSourceVisibilityHits<<<blocksForRaysShot, TPB, 0, stream>>>(
            d_visibilities,
            numVisibilities,
            d_selectedPatch,
            d_sourceHitCounts
        );
        CUDA_CHECK_STD_ERROR(cudaGetLastError());

        // Zlicz WSZYSTKIE promienie wystrzelone (bez limitu 1024 bloków)
        countSourceRaysShot<<<blocksForRaysShot, TPB, 0, stream>>>(
            d_visibilities,
            numVisibilities,
            d_selectedPatch,
            d_totalRaysShot
        );
        cudaEventRecord(stop, stream);
        cudaEventSynchronize(stop);
        cudaEventElapsedTime(&elapsedTime, start, stop);
        // std::cout << "CountSourceVisibilityHits kernel took " << elapsedTime << " ms\n";

        CUDA_CHECK_STD_ERROR(cudaGetLastError());

        cudaEventRecord(start, stream);
        calculateRadiosity<<<blocksForRaysShot, TPB, 0, stream>>>(
            d_patches,
            numPatches,
            d_visibilities,
            numVisibilities,
            d_selectedPatch,
            d_sourceHitCounts,
            d_totalRaysShot,
            d_lightMap
        );
        CUDA_CHECK_STD_ERROR(cudaGetLastError());
        cudaEventRecord(stop, stream);
        cudaEventSynchronize(stop);
        cudaEventElapsedTime(&elapsedTime, start, stop);
        // std::cout << "CalculateRadiosity kernel took " << elapsedTime << " ms\n";

        // Reset source patch energy after radiosity accumulation so the next selection pass sees the update.
        const uint32_t resetThreads = min(TPB, SELECTED_PATCHES_COUNT);
        const uint32_t resetBlocks = (SELECTED_PATCHES_COUNT + resetThreads - 1) / resetThreads;
        resetSelectedPatchUnshotEnergy<<<resetBlocks, resetThreads, 0, stream>>>(d_patches, numPatches, d_selectedPatch, d_sourceHitCounts);
        CUDA_CHECK_STD_ERROR(cudaGetLastError());

        cudaFree(d_sourceHitCounts);
        cudaFree(d_totalRaysShot);
    }

    __host__ void runInterpolateVertexKernel(Patch* d_patches, uint32_t numPatches,
                                    const uint32_t* d_vertexPatchIndices, const uint32_t* d_vertexPatchOffsets,
                                    uint32_t numVertices, glm::vec3* radVertexColors)
    {
        if (numPatches == 0 || numVertices == 0)
        {
            return;
        }

        int blocks = (numVertices + TPB - 1) / TPB;
        blocks = min(blocks, 1024);

        interpolateVertexColors<<<blocks, TPB>>>(d_patches, numPatches, d_vertexPatchIndices, d_vertexPatchOffsets, numVertices, radVertexColors);
        CUDA_CHECK_STD_ERROR(cudaGetLastError());
    }

    __host__ void topKPatches(uint32_t numPatches, float* d_energies, uint32_t* d_selectedPatchesId, cudaStream_t stream)
    {
        if (numPatches == 0)
        {
            return;
        }

        thrust::device_vector<uint32_t> indices(numPatches);
        thrust::sequence(thrust::cuda::par.on(stream), indices.begin(), indices.end());

        thrust::sort_by_key(
            thrust::cuda::par.on(stream),
            d_energies,
            d_energies + numPatches,
            indices.begin(),
            thrust::greater<float>()
        );

        // No w zasadzie SelectedPatch nie musi miec informacji o unshootEnergy,
        // to moze byc po prostu indeks patcha, a to unshot energy wezmie sie z tablicy patches
        CUDA_CHECK_STD_ERROR(cudaMemcpyAsync(
            d_selectedPatchesId,
            thrust::raw_pointer_cast(indices.data()),
            sizeof(uint32_t) * SELECTED_PATCHES_COUNT,
            cudaMemcpyDeviceToDevice,
            stream
        ));
    }
}
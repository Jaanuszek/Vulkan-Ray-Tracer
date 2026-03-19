#include "pch.h"
#include "Radiosity.cuh"

namespace VRTR::CUDA
{
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

    __global__ void filterSelectedPatches(const SelectedPatch* inSelected,
                                          SelectedPatch* outSelected,
                                          uint32_t numSelected)
    {
        uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;

        // extern __shared__ unsigned char sharedRaw[];
        // float* maxEnergy = reinterpret_cast<float*>(sharedRaw);
        // uint32_t* maxPatchId = reinterpret_cast<uint32_t*>(maxEnergy + blockDim.x);
        __shared__ float maxEnergy[TPB];
        __shared__ uint32_t maxPatchId[TPB];

        float localMaxEnergy = -FLT_MAX;
        uint32_t localMaxPatchId = 0;

        for (uint32_t i = idx; i < numSelected; i += blockDim.x * gridDim.x)
        {
            float e = inSelected[i].unshotEnergy;
            if (e > localMaxEnergy)
            {
                localMaxEnergy = e;
                localMaxPatchId = inSelected[i].patchId;
            }
        }

        maxEnergy[threadIdx.x] = localMaxEnergy;
        maxPatchId[threadIdx.x] = localMaxPatchId;

        __syncthreads();

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
            outSelected[blockIdx.x].patchId = maxPatchId[0];
            outSelected[blockIdx.x].unshotEnergy = maxEnergy[0];
        }
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

        // 🔹 buffer na wyniki bloków
        SelectedPatch* d_blockResults = nullptr;
        CUDA_CHECK_STD_ERROR(cudaMalloc(&d_blockResults, blocks * sizeof(SelectedPatch)));

        // 🔹 kernel
        filterPatches<<<blocks, TPB, 0, stream>>>(
            patches,
            numPatches,
            d_blockResults
        );

        CUDA_CHECK_STD_ERROR(cudaGetLastError());

        // 🔹 kopiujemy małą tablicę do CPU
        std::vector<SelectedPatch> h_blockResults(blocks);

        CUDA_CHECK_STD_ERROR(cudaMemcpyAsync(
            h_blockResults.data(),
            d_blockResults,
            blocks * sizeof(SelectedPatch),
            cudaMemcpyDeviceToHost,
            stream
        ));

        CUDA_CHECK_STD_ERROR(cudaStreamSynchronize(stream));

        SelectedPatch best;
        best.unshotEnergy = -FLT_MAX;
        best.patchId = 0;

        for (int i = 0; i < blocks; i++)
        {
            if (h_blockResults[i].unshotEnergy > best.unshotEnergy)
            {
                best = h_blockResults[i];
            }
        }

        CUDA_CHECK_STD_ERROR(cudaMemcpyAsync(
            selectedPatch,
            &best,
            sizeof(SelectedPatch),
            cudaMemcpyHostToDevice,
            stream
        ));

        cudaFree(d_blockResults);
        // if (numPatches == 0)
        // {
        //     return;
        // }

        // int blocks = (numPatches + TPB - 1) / TPB;

        // filterPatches<<<blocks, TPB, 0, stream>>>(patches, numPatches, selectedPatch);
        // cudaError_t err = cudaGetLastError();
        // if (err != cudaSuccess)
        // {
        //     std::cerr << "CUDA kernel launch error: " << cudaGetErrorString(err) << std::endl;
        //     return;
        // }

        // //TODO tutaj mi czat jakies cos wygenerowal i mi sie to nie podoba bo jest kilka
        // // alokacji w petli

        // // Iteracyjna redukcja wyników bloków do pojedynczego SelectedPatch.
        // uint32_t currentCount = static_cast<uint32_t>(blocks);
        // SelectedPatch* currentIn = selectedPatch;

        // while (currentCount > 1)
        // {
        //     uint32_t reduceBlocks = (currentCount + TPB - 1) / TPB;

        //     SelectedPatch* nextOut = nullptr;
        //     CUDA_CHECK_STD_ERROR(cudaMalloc(&nextOut, reduceBlocks * sizeof(SelectedPatch)));

        //     filterSelectedPatches<<<reduceBlocks, TPB, 0, stream>>>(currentIn, nextOut, currentCount);

        //     err = cudaGetLastError();
        //     if (err != cudaSuccess)
        //     {
        //         std::cerr << "CUDA kernel launch error: " << cudaGetErrorString(err) << std::endl;
        //         cudaFree(nextOut);
        //         if (currentIn != selectedPatch)
        //         {
        //             cudaFree(currentIn);
        //         }
        //         return;
        //     }

        //     if (currentIn != selectedPatch)
        //     {
        //         cudaFree(currentIn);
        //     }

        //     currentIn = nextOut;
        //     currentCount = reduceBlocks;
        // }

        // if (currentIn != selectedPatch)
        // {
        //     CUDA_CHECK_STD_ERROR(cudaMemcpyAsync(selectedPatch, currentIn, sizeof(SelectedPatch), cudaMemcpyDeviceToDevice, stream));
        //     cudaFree(currentIn);
        // }
    }
}
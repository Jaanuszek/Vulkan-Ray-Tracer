#include "pch.h"
#include "Radiosity.cuh"

namespace VRTR::CUDA
{
    __global__ void filterPatches(Patch* patches, uint32_t numPatches,
                                  SelectedPatch* selectedPatch)
    {
        uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;

        // 1024 bo moze byc max 1024 threadów per block
        __shared__ float maxEnergy[1024];
        __shared__ uint32_t maxPatchId[1024];

        float localMaxEnergy = 0.0f;
        uint32_t localMaxPatchId = 0;
        /*
            Pętla iterująca po patchach w taki sposób,
            że każdy thread przetwarza kilka patchy (tzw. striding).
             - idx to globalny indeks threada
             - gridDim.x * blockDim.x to całkowita liczba threadów w siatce
             - i += gridDim.x * blockDim.x powoduje, że każdy thread przeskakuje o tyle patchy, ile jest threadów, 
               co pozwala równomiernie rozłożyć pracę nawet jeśli liczba patchy nie jest idealnie podzielna przez liczbę threadów.
        */
        for (uint32_t i = idx; i < numPatches; i += gridDim.x * blockDim.x)
        {
            if (patches[i].unshotEnergy > localMaxEnergy)
            {
                localMaxEnergy = patches[i].unshotEnergy;
                // Uzywamy globalnego ID patcha, a nie indeksu w buforze.
                localMaxPatchId = patches[i].id;
            }
        }

        // Zapisz dane do shared memory

        maxEnergy[threadIdx.x] = localMaxEnergy;
        maxPatchId[threadIdx.x] = localMaxPatchId;
        __syncthreads(); // obowiązkowo synchronizacja zeby wszystkie wątki zdążyły zapisać

        // Redukcja w ramach bloku (tree reduction)
        for (uint32_t s = blockDim.x / 2; s > 0; s >>= 1)
        {
            if(threadIdx.x < s)
            {
                // porównanie pierwszego elementu z elementem oddalonym o s, czyli o połowe block dim w pierwszej iteracji
                if (maxEnergy[threadIdx.x + s] > maxEnergy[threadIdx.x])
                {
                    maxEnergy[threadIdx.x] = maxEnergy[threadIdx.x + s];
                    maxPatchId[threadIdx.x] = maxPatchId[threadIdx.x + s];
                }
            }
            __syncthreads(); 
        }

        // Po tym maxEnergy[0] to jest maksymalna niewystrzelona energia z patcha

        if (threadIdx.x == 0)
        {
            unsigned int* globalEnergyBits = reinterpret_cast<unsigned int*>(&selectedPatch->unshotEnergy);
            unsigned int candidateEnergyBits = __float_as_uint(maxEnergy[0]);
            unsigned int previousEnergyBits = atomicMax(globalEnergyBits, candidateEnergyBits);

            // patchId zapisuje tylko blok, ktory rzeczywiscie podniosl globalne maksimum.
            if (candidateEnergyBits > previousEnergyBits)
            {
                selectedPatch->patchId = maxPatchId[0];
            }
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
        {
            return;
        }

        int threadsPerBlock = 256;
        int blocks = (numPatches + threadsPerBlock - 1) / threadsPerBlock;

        filterPatches<<<blocks, threadsPerBlock, 0, stream>>>(patches, numPatches, selectedPatch);
        cudaError_t err = cudaGetLastError();
        if (err != cudaSuccess)
        {
            std::cerr << "CUDA kernel launch error: " << cudaGetErrorString(err) << std::endl;
        }
    }
}
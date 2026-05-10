# Implementacja Radiosity - Przewodnik Integracji

## Architektura

```
┌─────────────────────────────────────────────────────────────────────┐
│ Per Frame / Per Iteration:                                           │
├─────────────────────────────────────────────────────────────────────┤
│                                                                       │
│ 1. CUDA: filterPatches()                                             │
│    ├─ Input: Patch[] (z unshotEnergy)                               │
│    └─ Output: SelectedPatch (patchId z max energy)                  │
│                                                                       │
│ 2. VULKAN TIMELINE SEMAPHORE SYNC                                    │
│    └─ Czekamy aż CUDA skończy                                       │
│                                                                       │
│ 3. VULKAN: Visibility Ray Tracing Pass                               │
│    ├─ Input: SelectedPatch.patchId (emitter)                        │
│    ├─ Ray tracing po to check which patches can see selectedPatch   │
│    └─ Output: PatchVisibility[] (wypełnione z visibility=0/1)       │
│                                                                       │
│ 4. VULKAN TIMELINE SEMAPHORE SYNC                                    │
│    └─ Czekamy aż Vulkan skończy                                     │
│                                                                       │
│ 5. CUDA: calculateRadiosity()                                        │
│    ├─ Input: Patch[], PatchVisibility[], srcPatchId                 │
│    └─ Output: Patch[].radiosity += calculated energy                │
│                                                                       │
│ 6. CUDA: Zero unsegunshot energy dla srcPatch                        │
│    └─ selectedPatch.unshotEnergy = 0                                │
│                                                                       │
│ 7. Repeat until convergence                                          │
│                                                                       │
└─────────────────────────────────────────────────────────────────────┘
```

## Implementacja Vulkan Visibility Pass

Będziesz potrzebować:

### 1. Ray Tracing Pipeline dla Visibility

Utwórz nowy Ray Tracing Pipeline z:
- **Closest hit shader** - zwraca 1.0 (visible)
- **Miss shader** - zwraca 0.0 (not visible)

```glsl
// closest_hit.rchit
layout(location = 0) rayPayloadEXT vec3 pld;
void main() {
    pld.x = 1.0; // visible
}

// miss.rmiss
layout(location = 0) rayPayloadEXT vec3 pld;
void main() {
    pld.x = 0.0; // not visible
}
```

### 2. Descriptor Set dla Radiosity

```cpp
// W DescriptorManager dodaj:
struct RadiosityDescriptorSet {
    VkDescriptorSet set;
    // Buffers:
    Buffer* patchesBuffer;        // R/W - Patch[]
    Buffer* selectedPatchBuffer;  // R/W - SelectedPatch
    Buffer* visibilityBuffer;     // W - PatchVisibility[]
    // Obrazy:
    StorageImage* lightmapImage;  // R/W - lightmap RGB
};
```

### 3. Visibility Ray Tracing Shader

```glsl
#version 460
#extension GL_EXT_ray_tracing : require

layout(binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1, std430) buffer PatchesBuffer { Patch[] patches; };
layout(binding = 2, std430) buffer SelectedBuffer { SelectedPatch selected; };
layout(binding = 3, std430) buffer VisibilityBuffer { PatchVisibility[] visibilities; };

layout(location = 0) rayPayloadEXT float visibility;

void main() {
    uint patchIdx = gl_LaunchIDEXT.x;
    
    if (patchIdx >= patches.length()) return;
    
    Patch srcPatch = patches[selected.patchId];
    Patch dstPatch = patches[patchIdx];
    
    // Ray z srcPatch do dstPatch
    vec3 rayOrigin = srcPatch.center;
    vec3 rayDir = normalize(dstPatch.center - srcPatch.center);
    float rayDist = distance(srcPatch.center, dstPatch.center);
    
    // Raycasta  
    visibility = 0.0;
    traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0,
                rayOrigin, 0.001, rayDir, rayDist - 0.001,
                0);
    
    // Zapisz wynik
    visibilities[patchIdx].srcPatchId = selected.patchId;
    visibilities[patchIdx].dstPatchId = patchIdx;
    visibilities[patchIdx].visibility = visibility;
}
```

### 4. Integracja z vkCudaInterop::runCudaFrame()

```cpp
void vkCudaInterop::radiosity_iteration()
{
    // 1. CUDA: Filter patches
    {
        uint64_t cudaSemWait = vkToCudaSignalValue;
        uint64_t cudaSemSignal = vkToCudaSignalValue + 1;
        
        waitForSemapore(cudaSemWait);
        
        // Kernel call:
        CUDA::filterPatches<<<grid, block, 0, cudaStream>>>(
            cudaPatchesData, numPatches, &cudaSelectedPatch);
        
        signalSemaphore(cudaSemSignal);
        cudaToVkWaitValue = cudaSemSignal;
        vkToCudaSignalValue += 2;
    }
    
    // 2. Vulkan visibility pass (uruchamia się w main command buffer)
    // siehe: RTRenderer::radiosity_visibility_pass()
    
    // 3. CUDA: Calculate radiosity
    {
        uint64_t cudaSemWait = vkToCudaSignalValue;
        uint64_t cudaSemSignal = vkToCudaSignalValue + 1;
        
        waitForSemapore(cudaSemWait);
        
        // Kernel call:
        CUDA::calculateRadiosity<<<grid, block, 0, cudaStream>>>(
            cudaPatchesData, numPatches, 
            cudaVisibilityData, numPatches,
            selectedPatch.patchId);
        
        // Zero unshot energy dla selected patcha
        CUDA::zeroUnshotEnergy<<<1, 1, 0, cudaStream>>>(
            cudaPatchesData, selectedPatch.patchId);
        
        signalSemaphore(cudaSemSignal);
        cudaToVkWaitValue = cudaSemSignal;
        vkToCudaSignalValue += 2;
    }
}
```

## Synchronizacja CUDA <-> Vulkan

### Timeline Semaphore Pattern

```
Frame N:
├─ Vulkan: Signal semaphore(N)
├─ CUDA: Wait(N), Execute, Signal(N+1)
├─ Vulkan: Signal semaphore(N+1)
├─ CUDA: Wait(N+1), Execute, Signal(N+2)
└─ Vulkan: Signal semaphore(N+2)
```

Już masz to Setup w `vkCudaInterop`. Pamiętaj aby:
1. Zawsze czekać na semaphore przed operacją
2. Signalizować po operacji
3. Inkrementować wartości semaphore

## Dane wspólne (Shared Buffers)

Potrzebujesz Import tych buforów:

```cpp
// W vkCudaInterop::init():

// 1. Patches buffer
vk::DeviceSize patchesBuffSize = numPatches * sizeof(Patch);
patchesBuffer = std::make_unique<Buffer>(ctx, patchesBuffSize,
    vk::BufferUsageFlagBits::eTransferDst | 
    vk::BufferUsageFlagBits::eStorageBuffer,
    vk::MemoryPropertyFlagBits::eDeviceLocal,
    vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd);

auto devMem = patchesBuffer->getBufferMemory();
CUDA::importCudaExternalMemory(ctx.logicalDevice, (void**)&cudaPatchesData,
    cudaPatchesExternalMemory, devMem, patchesBuffSize,
    vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd);

// 2. Selected patch buffer
// Similar pattern...

// 3. Visibility buffer
// Similar pattern...
```

## Debugowanie

1. **CPU readback** - co kilka iteracji przeczytaj Patch[] na CPU i sprawdzić energia
2. **Visualization** - renderuj Radiosity jako heatmap zamiast lightmapy
3. **Assertions** - sprawdzaj że visibility jest w [0, 1]

## Performance Notes

- `filterPatches` - O(numPatches) w shared memory reduction ✓
- `calculateRadiosity` - O(numVisibilities) z atomic operacjami
  - Może być bottleneck gdy wiele threads pisze do tego samego patcha
  - Rozwiązanie: reduce per-block, potem atomic per-patch
- Ray tracing pass - najdroższy, ale niezbęny

## TODO dla ciebie

- [ ] Implement RTRenderer::radiosity_visibility_pass()
- [ ] Dodaj radiosity loop w main render loop
- [ ] Dodaj lightmap texture binding
- [ ] Test synchronizacji CUDA-Vulkan
- [ ] Zoptymalizuj calculateRadiosity reduce pattern

#pragma once

#include <cuda_runtime_api.h>
#include <memory.h>
#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include <cmath>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include "Logger.hpp"

// "#" w define zamienia argument na string
#define CUDA_CHECK_ERROR(err) checkCudaError(err,  #err, __FILE__, __LINE__)

inline void checkCudaError(cudaError_t err, const char* msg, const char* file, const int line)
{
    if (err != cudaSuccess)
    {
        VRTR_ERROR("CUDA Error: {}: {} from file {}, line {}", msg, cudaGetErrorString(err), file, line);
        exit(EXIT_FAILURE);
    }
}

namespace VRTR
{
    namespace CUDA
    {
        int initCUDA(uint8_t *vkDeviceUUID, size_t UUID_SIZE);

        // TODO kurde to chyba powinno byc w kodzie vulkanowym
        void *getMemHandle(vk::DeviceMemory vkMem, vk::ExternalMemoryHandleTypeFlagBits handleType);

        void importCudaExternalMemory(void **cudaPtr, cudaExternalMemory_t &cudaMem,
                                      vk::DeviceMemory &vkMem, vk::DeviceSize size,
                                      vk::ExternalMemoryHandleTypeFlags handleType);
    }
}
#include "pch.h"
#include "init_cuda.cuh"

namespace VRTR::CUDA
{
    int initCUDA(uint8_t *vkDeviceUUID, size_t UUID_SIZE)
    {
        VRTR_DEBUG("CUDA INIT");
        int currentDevice{};
        int deviceCount{};
        int devicesProhibited{};

        cudaDeviceProp deviceProp;

        CUDA_CHECK_ERROR(cudaGetDeviceCount(&deviceCount));
        if(deviceCount == 0)
        {
            VRTR_ERROR("No CUDA devices found");
            exit(EXIT_FAILURE);
        }

        while (currentDevice < deviceCount)
        {
            int computeMode{};
            CUDA_CHECK_ERROR(cudaGetDeviceProperties(&deviceProp, currentDevice));
            CUDA_CHECK_ERROR(cudaDeviceGetAttribute(&computeMode, cudaDeviceAttr::cudaDevAttrComputeMode, currentDevice));
            if(computeMode != cudaComputeModeProhibited)
            {
                int ret = memcmp(deviceProp.uuid.bytes, vkDeviceUUID, UUID_SIZE);
                if(ret == 0)
                {
                    CUDA_CHECK_ERROR(cudaSetDevice(currentDevice));
                    // czy to jest potrzebne?
                    CUDA_CHECK_ERROR(cudaGetDeviceProperties(&deviceProp, currentDevice));
                    VRTR_DEBUG("CUDA device selected: {}: \"{}\" with compute capability {}.{}",
                         currentDevice, deviceProp.name, deviceProp.major, deviceProp.minor);
                    return currentDevice;
                }
            }
            else
            {
                devicesProhibited++;
            }

            currentDevice++;
        }
        if(devicesProhibited == deviceCount)
        {
            VRTR_ERROR("All CUDA devices are prohibited");
            exit(EXIT_FAILURE);
        }

        return -1;
    }

    void *getMemHandle(vk::Device logDevice, vk::DeviceMemory vkMem, vk::ExternalMemoryHandleTypeFlagBits handleType)
    {
    #ifdef _WIN64
        VRTR_ERROR("Windows platform is not yet supported");
        exit(EXIT_FAILURE);
    #else
        int fd = -1;
        vk::MemoryGetFdInfoKHR fdInfo{
            .pNext = nullptr,
            .memory = vkMem,
            .handleType = handleType,
        };

        /* 
            getMemoryFdKHR tworzy file descriptor dla pamieci ktora bedzie exportowana
            Jezeli pamięc zostanie wyeksportowana, to ownership jest przekazywany
            impord fd do cudy -> cuda przejmuje ownership -> cuda zwalnia pamięć
            Jezeli nie nastąpi import, to trzeba to fd zwolnic w vulkanie 
        */

        logDevice.getMemoryFdKHR(&fdInfo, &fd); // <--- to za kazdym razem daje inny fd, nawet dla tego samego buffora
        return (void *)(uintptr_t)fd;
    #endif
    }

    void *getSemHandle(vk::Device logDevice, vk::Semaphore vkSem, vk::ExternalSemaphoreHandleTypeFlagBits handleType)
    {
        #ifdef _WIN64
            VRTR_ERROR("Windows platform is not yet supported");
            exit(EXIT_FAILURE);
        #else
            int fd = -1;
            vk::SemaphoreGetFdInfoKHR fdInfo{
                .pNext = nullptr,
                .semaphore = vkSem,
                .handleType = handleType,
            };

            logDevice.getSemaphoreFdKHR(&fdInfo, &fd);
            return (void *)(uintptr_t)fd;
        #endif
    }

    void importCudaExternalMemory(vk::Device logDevice,
                                void **cudaPtr, cudaExternalMemory_t &cudaMem,
                                vk::DeviceMemory &vkMem, vk::DeviceSize size,
                                vk::ExternalMemoryHandleTypeFlagBits handleType)
    {
        cudaExternalMemoryHandleDesc externalMemoryHandleDesc{};
        if(handleType & vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd)
        {
            externalMemoryHandleDesc.type = cudaExternalMemoryHandleTypeOpaqueFd;
        } else {
            VRTR_ERROR("Unsupported external memory handle type");
            exit(EXIT_FAILURE);
        }

        externalMemoryHandleDesc.size = size;
        externalMemoryHandleDesc.handle.fd = (int)(uintptr_t)getMemHandle(logDevice, vkMem, handleType);

        CUDA_CHECK_ERROR(cudaImportExternalMemory(&cudaMem, &externalMemoryHandleDesc));

        cudaExternalMemoryBufferDesc extenralMemBufferDesc{
            .offset = 0,
            .size = size,
            .flags = 0
        };

        // cudaPTR musi byc zwolnione korzystajac z cudaFree()!!!!
        CUDA_CHECK_ERROR(cudaExternalMemoryGetMappedBuffer(cudaPtr, cudaMem, &extenralMemBufferDesc));
    }

    void importCudaExternalSemaphore(vk::Device logDevice,
                                    cudaExternalSemaphore_t &cudaSem,
                                    vk::Semaphore vkSem,
                                    vk::ExternalSemaphoreHandleTypeFlagBits handleType)
    {
        cudaExternalSemaphoreHandleDesc externalSemaphoreHandleDesc{};
#ifdef VK_TIMELINE_SEMAPHORE
        if(handleType & vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd)
        {
            externalSemaphoreHandleDesc.type = cudaExternalSemaphoreHandleTypeTimelineSemaphoreFd;
        }
#else
        if(handleType & vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd)
        {
            externalSemaphoreHandleDesc.type = cudaExternalSemaphoreHandleTypeOpaqueFd;
        }
#endif
        else
        {
            VRTR_ERROR("Unsupported external semaphore handle type");
            exit(EXIT_FAILURE);
        }
        #ifdef _WIN64
            VRTR_ERROR("Windows platform is not yet supported");
            exit(EXIT_FAILURE);
        #else
            externalSemaphoreHandleDesc.handle.fd = (int)(uintptr_t)getSemHandle(logDevice, vkSem, handleType);
            CUDA_CHECK_ERROR(cudaImportExternalSemaphore(&cudaSem, &externalSemaphoreHandleDesc));
        #endif
    }
}
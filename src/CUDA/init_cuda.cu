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

    void importCudaExternalMemory(void **cudaPtr, cudaExternalMemory_t &cudaMem,
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
        externalMemoryHandleDesc.handle.fd = (int)(uintptr_t)getMemHandle(vkMem, handleType);
    }
}
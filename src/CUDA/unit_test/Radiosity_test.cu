#include "pch.h"
#include "Radiosity_test.cuh"


namespace VRTR::CUDA
{
    static bool ensureCudaReadyForTests()
    {
        // Forces CUDA runtime initialization and surfaces driver/runtime issues early.
        cudaError_t initErr = cudaFree(nullptr);
        if (initErr != cudaSuccess)
        {
            return false;
        }

        int deviceCount = 0;
        cudaError_t countErr = cudaGetDeviceCount(&deviceCount);
        if (countErr != cudaSuccess)
        {
            return false;
        }

        return deviceCount > 0;
    }

    // ========================================================================
    // Helper Utility Functions for Tests
    // ========================================================================
    
    // Helper function to allocate and copy data to GPU
    template<typename T>
    T* allocateAndCopyToGPU(const T* hostData, size_t count)
    {
        T* deviceData = nullptr;
        cudaError_t allocErr = cudaMalloc(&deviceData, count * sizeof(T));
        if (allocErr != cudaSuccess)
        {
            std::cerr << "cudaMalloc failed: " << cudaGetErrorString(allocErr) << std::endl;
            return nullptr;
        }

        if (hostData)
        {
            cudaError_t copyErr = cudaMemcpy(deviceData, hostData, count * sizeof(T), cudaMemcpyHostToDevice);
            if (copyErr != cudaSuccess)
            {
                std::cerr << "cudaMemcpy(H2D) failed: " << cudaGetErrorString(copyErr) << std::endl;
                cudaFree(deviceData);
                return nullptr;
            }
        }
        return deviceData;
    }
    
    // Helper function to copy data from GPU to Host
    template<typename T>
    void copyFromGPU(T* hostData, const T* deviceData, size_t count)
    {
        cudaMemcpy(hostData, deviceData, count * sizeof(T), cudaMemcpyDeviceToHost);
    }
    
    // ========================================================================
    // Test Fixture - Common setup for all radiosity tests
    // ========================================================================
    
    class RadiosityKernelTest : public ::testing::Test
    {
    protected:
        static constexpr uint32_t NUM_PATCHES = 2048; // wiecej niz 1024 zeby przetestowac wiele blokow
        uint32_t BlockCount = (NUM_PATCHES + TPB - 1) / TPB;

        std::random_device rd;
        std::mt19937 gen{rd()};
        
        std::vector<Patch> hostPatches;
        Patch* d_patches = nullptr;
        SelectedPatch* d_selectedPatch = nullptr;
        
        void SetUp() override
        {
            if (!ensureCudaReadyForTests())
            {
                GTEST_SKIP() << "CUDA runtime/driver/device is not available: "
                             << cudaGetErrorString(cudaGetLastError());
            }

            std::uniform_real_distribution<float> unshootEnergyDist(0.0f, 1.0f);
            // Initialize CPU patches with test data
            hostPatches.resize(NUM_PATCHES);
            for (uint32_t i = 0; i < NUM_PATCHES; ++i)
            {
                hostPatches[i].id = i;
                hostPatches[i].area = 1.0f;
                hostPatches[i].emission = 0.5f;
                hostPatches[i].center = glm::vec3(i, i, i);
                hostPatches[i].normal = glm::vec3(0, 1, 0);
                hostPatches[i].albedo = glm::vec3(0.8f, 0.8f, 0.8f);
                hostPatches[i].unshotEnergy = unshootEnergyDist(gen);
                hostPatches[i].radiosity = 0.0f;
            }
            
            // Allocate GPU memory and copy data
            d_patches = allocateAndCopyToGPU(hostPatches.data(), NUM_PATCHES);
            ASSERT_NE(d_patches, nullptr);
            
            SelectedPatch initialSelected = {0, 0.0f};
            d_selectedPatch = allocateAndCopyToGPU(&initialSelected, static_cast<size_t>(BlockCount));
            ASSERT_NE(d_selectedPatch, nullptr);
        }
        
        void TearDown() override
        {
            if (d_patches)
            {
                cudaFree(d_patches);
                d_patches = nullptr;
            }
            if (d_selectedPatch)
            {
                cudaFree(d_selectedPatch);
                d_selectedPatch = nullptr;
            }
        }
    };
    
    // ========================================================================
    // Test Cases
    // ========================================================================
    
    TEST_F(RadiosityKernelTest, FilterPatchesSelectsMaxEnergy)
    {
        ASSERT_EQ(cudaMemset(d_selectedPatch, 0, sizeof(SelectedPatch) * BlockCount), cudaSuccess);
        runFilterPatchesKernel(d_patches, NUM_PATCHES, d_selectedPatch, 0);
        ASSERT_EQ(cudaGetLastError(), cudaSuccess);
        ASSERT_EQ(cudaDeviceSynchronize(), cudaSuccess);

        std::vector<SelectedPatch> result(BlockCount);
        ASSERT_EQ(cudaMemcpy(result.data(), d_selectedPatch, sizeof(SelectedPatch) * BlockCount, cudaMemcpyDeviceToHost), cudaSuccess);
        
        float cpuMaxUnshoot = std::max_element(hostPatches.begin(), hostPatches.end(), [](const Patch& a, const Patch&b) {
            return a.unshotEnergy < b.unshotEnergy;
        })->unshotEnergy;

        EXPECT_FLOAT_EQ(result[0].unshotEnergy, cpuMaxUnshoot);
    }
    
    TEST_F(RadiosityKernelTest, FilterPatchesHandlesSinglePatch)
    {
        float testEnergy = 0.8f;
        std::vector<Patch> singlePatch(1);
        singlePatch[0].id = 0;
        singlePatch[0].unshotEnergy = testEnergy;
        singlePatch[0].center = glm::vec3(0, 0, 0);
        singlePatch[0].normal = glm::vec3(0, 1, 0);
        
        Patch* d_singlePatch = allocateAndCopyToGPU(singlePatch.data(), 1);
        SelectedPatch initialSelected = {0, 0.0f};
        SelectedPatch* d_single_selected = allocateAndCopyToGPU(&initialSelected, 1);
        
        // Act
        ASSERT_EQ(cudaMemset(d_single_selected, 0, sizeof(SelectedPatch)), cudaSuccess);
        runFilterPatchesKernel(d_singlePatch, 1, d_single_selected, 0);
        ASSERT_EQ(cudaGetLastError(), cudaSuccess);
        ASSERT_EQ(cudaDeviceSynchronize(), cudaSuccess);
        
        // Assert
        SelectedPatch result{};
        ASSERT_EQ(cudaMemcpy(&result, d_single_selected, sizeof(SelectedPatch), cudaMemcpyDeviceToHost), cudaSuccess);
        
        EXPECT_EQ(result.patchId, 0);
        EXPECT_FLOAT_EQ(result.unshotEnergy, testEnergy);
        
        // Cleanup
        cudaFree(d_singlePatch);
        cudaFree(d_single_selected);
    }
    
    TEST_F(RadiosityKernelTest, FilterPatchesHandlesZeroEnergy)
    {
        // Arrange - All patches have zero energy
        std::vector<Patch> zeroEnergyPatches(10);
        for (size_t i = 0; i < zeroEnergyPatches.size(); ++i)
        {
            zeroEnergyPatches[i].id = i;
            zeroEnergyPatches[i].unshotEnergy = 0.0f;
            zeroEnergyPatches[i].center = glm::vec3(0, 0, 0);
            zeroEnergyPatches[i].normal = glm::vec3(0, 1, 0);
        }
        
        Patch* d_zeroPatch = allocateAndCopyToGPU(zeroEnergyPatches.data(), zeroEnergyPatches.size());
        SelectedPatch initialSelected = {0, 0.0f};
        SelectedPatch* d_zero_selected = allocateAndCopyToGPU(&initialSelected, 1);
        
        // Act
        ASSERT_EQ(cudaMemset(d_zero_selected, 0, sizeof(SelectedPatch)), cudaSuccess);
        runFilterPatchesKernel(d_zeroPatch, 10, d_zero_selected, 0);
        ASSERT_EQ(cudaGetLastError(), cudaSuccess);
        ASSERT_EQ(cudaDeviceSynchronize(), cudaSuccess);
        
        // Assert
        SelectedPatch result{};
        ASSERT_EQ(cudaMemcpy(&result, d_zero_selected, sizeof(SelectedPatch), cudaMemcpyDeviceToHost), cudaSuccess);
        
        EXPECT_FLOAT_EQ(result.unshotEnergy, 0.0f);
        
        // Cleanup
        cudaFree(d_zeroPatch);
        cudaFree(d_zero_selected);
    }
    
    TEST_F(RadiosityKernelTest, FilterPatchesWithLargeDataset)
    {
        // Arrange - Large number of patches
        constexpr uint32_t LARGE_NUM = 10000;
        uint32_t BlockCount = (LARGE_NUM + TPB - 1) / TPB;
        std::vector<Patch> largePatches(LARGE_NUM);
        for (uint32_t i = 0; i < LARGE_NUM; ++i)
        {
            largePatches[i].id = i;
            largePatches[i].unshotEnergy = static_cast<float>(i) * 0.001f;
            largePatches[i].center = glm::vec3(i, i, i);
            largePatches[i].normal = glm::vec3(0, 1, 0);
        }
        
        Patch* d_largePatches = allocateAndCopyToGPU(largePatches.data(), LARGE_NUM);
        SelectedPatch initialSelected = {0, 0.0f};
        SelectedPatch* d_large_selected = allocateAndCopyToGPU(&initialSelected, static_cast<size_t>(BlockCount));
        
        // Act
        ASSERT_EQ(cudaMemset(d_large_selected, 0, sizeof(SelectedPatch) * BlockCount), cudaSuccess);
        runFilterPatchesKernel(d_largePatches, LARGE_NUM, d_large_selected, 0);
        ASSERT_EQ(cudaGetLastError(), cudaSuccess);
        ASSERT_EQ(cudaDeviceSynchronize(), cudaSuccess);
        
        // Assert
        std::vector<SelectedPatch> result(BlockCount);
        ASSERT_EQ(cudaMemcpy(result.data(), d_large_selected, sizeof(SelectedPatch) * BlockCount, cudaMemcpyDeviceToHost), cudaSuccess);
        
        EXPECT_EQ(result[0].patchId, LARGE_NUM - 1);
        EXPECT_FLOAT_EQ(result[0].unshotEnergy, static_cast<float>(LARGE_NUM - 1) * 0.001f);
        
        // Cleanup
        cudaFree(d_largePatches);
        cudaFree(d_large_selected);
    }
    
    // ========================================================================
    // Additional Test Cases - Specific Radiosity Scenarios
    // ========================================================================
    
    TEST_F(RadiosityKernelTest, VerifyKernelExecution)
    {
        // Simple test to verify kernel compiles and runs without errors
        ASSERT_EQ(cudaMemset(d_selectedPatch, 0, sizeof(SelectedPatch) * BlockCount), cudaSuccess);
        runFilterPatchesKernel(d_patches, NUM_PATCHES, d_selectedPatch, 0);
        EXPECT_EQ(cudaGetLastError(), cudaSuccess);
        EXPECT_EQ(cudaDeviceSynchronize(), cudaSuccess);
    }
    
} // namespace VRTR::CUDA

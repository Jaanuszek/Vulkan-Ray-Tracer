#include <cuda_runtime_api.h>
#include <memory.h>
#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include <cmath>
#include "CommonStructs.h"
#include "Radiosity.cuh"

#include <gtest/gtest.h>

constexpr uint32_t TPB = 1024;

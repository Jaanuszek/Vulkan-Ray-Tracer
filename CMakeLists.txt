cmake_minimum_required(VERSION 3.28)

project(VulkanRayTracer LANGUAGES CXX CUDA)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
# workaround zeby system brał VULKAN_SDK zamiast includów z systemowego Vulkana ktory jest w starszej wersji
# bez tego binarka sie nie chce uruchomić
include_directories(BEFORE SYSTEM ${Vulkan_INCLUDE_DIRS})

find_package(glfw3 REQUIRED)

find_package(CUDAToolkit REQUIRED)

find_package(CCCL QUIET CONFIG)
if(NOT CCCL_FOUND)
    find_package(CCCL REQUIRED CONFIG
        HINTS
            "${CUDAToolkit_TARGET_DIR}/lib/cmake/cccl"
            "${CUDAToolkit_LIBRARY_ROOT}/cmake/cccl"
            "${CUDAToolkit_ROOT}/targets/x86_64-linux/lib/cmake/cccl"
            "/usr/local/cuda/targets/x86_64-linux/lib/cmake/cccl"
            "/usr/local/cuda-13.1/targets/x86_64-linux/lib/cmake/cccl"
    )
endif()

set(CMAKE_CUDA_ARCHITECTURES 89)

#internal
add_subdirectory(src/common)
add_subdirectory(src/core)
add_subdirectory(src/engine)
add_subdirectory(src/renderer)
add_subdirectory(third-party/googletest)
add_subdirectory(src/CUDA)

SET(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)

SET(temp="${CMAKE_CURRENT_SOURCE_DIR}/shaders")
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME} 
    src/application/main.cpp
    ${IMGUI_SOURCES}
    ${IMGUI_BACKEND}
)

# add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_shaders)

target_link_libraries(${PROJECT_NAME} 
    Vulkan::Vulkan
    glfw
    Core
    common
    renderer
    cuda_lib
    gtest_main
)
# Exe needs to have different precompiled headers because of usage of different compiler flags
target_precompile_headers(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/src/common/pch.h")

set_target_properties(${PROJECT_NAME} PROPERTIES
    BUILD_RPATH "$ORIGIN/lib"
    INSTALL_RPATH "$ORIGIN/lib"
)

cmake_minimum_required(VERSION 3.28)

project(VulkanRayTracer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

#internal
add_subdirectory(src/common)
add_subdirectory(src/core)
add_subdirectory(src/engine)
add_subdirectory(src/renderer)

SET(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)

add_executable(${PROJECT_NAME} src/application/main.cpp)

target_link_libraries(${PROJECT_NAME} 
    Vulkan::Vulkan
    glfw
    Core
    common
    renderer
)
# Exe needs to have different precompiled headers because of usage of different compiler flags
target_precompile_headers(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/src/common/pch.h")

set_target_properties(${PROJECT_NAME} PROPERTIES
    BUILD_RPATH "$ORIGIN/lib"
    INSTALL_RPATH "$ORIGIN/lib"
)
# SET(CUDA_LIBRARY_NAME cuda_lib)
cmake_minimum_required(VERSION 3.28)

project(cuda_lib VERSION 1.0.0 DESCRIPTION "CUDA library for Vulkan Ray Tracer")

add_library(${PROJECT_NAME}
    SHARED
    init_cuda.cu
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(${PROJECT_NAME}
    # common
    Core
    CUDA::cudart
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/lib
)

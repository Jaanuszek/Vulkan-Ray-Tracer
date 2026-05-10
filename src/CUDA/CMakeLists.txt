# SET(CUDA_LIBRARY_NAME cuda_lib)
cmake_minimum_required(VERSION 3.28)

project(cuda_lib VERSION 1.0.0 DESCRIPTION "CUDA library for Vulkan Ray Tracer")

SET(PATH_TO_TESTS "${CMAKE_CURRENT_SOURCE_DIR}/unit_test")
SET(PATH_TO_TESTS_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/unit_test/include")

add_library(${PROJECT_NAME}
    SHARED
    init_cuda.cu
    Radiosity.cu
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(${PROJECT_NAME}
    common
    Core
    CUDA::cudart
    # Trust::CUDA
    CCCL::CCCL
    Vulkan::Vulkan
)

target_precompile_headers(${PROJECT_NAME} REUSE_FROM common)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}
    ${PATH_TO_TESTS_INCLUDE}
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/lib
)

# ============================================================================
# Unit Tests Configuration
# ============================================================================
set(TEST_EXECUTABLE_NAME cuda_tests)
set(TEST_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/unit_test")

include(GoogleTest)

add_executable(${TEST_EXECUTABLE_NAME}
    ${TEST_SOURCE_DIR}/Radiosity_test.cu
    Radiosity.cu
)

set_target_properties(${TEST_EXECUTABLE_NAME} PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(${TEST_EXECUTABLE_NAME}
    common
    CUDA::cudart
    Vulkan::Vulkan
    gtest
    gtest_main
)

target_include_directories(${TEST_EXECUTABLE_NAME}
    PRIVATE
    ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}
    ${TEST_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_precompile_headers(${TEST_EXECUTABLE_NAME} REUSE_FROM common)

# Register tests with CTest
gtest_discover_tests(${TEST_EXECUTABLE_NAME} DISCOVERY_MODE PRE_TEST)

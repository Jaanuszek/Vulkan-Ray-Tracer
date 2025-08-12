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

# set(SLANGC_EXECUTABLE "${VULKAN_SDK}/bin/slangc")

# function (add_slang_shader_target TARGET)
#     cmake_parse_arguments("SHADER" "" "SOURCES" ${ARGN})
#     set (SHADERS_DIR "${CMAKE_ROOT_SOURCE_DIR}/shaders")
#     set (ENTRY_POINTS -entry vertMain -entry fragMain)
#     add_custom_command(
#         OUTPUT ${SHADERS_DIR}
#         COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
#     )

#     MESSAGE(STATUS "================" ${ENTRY_POINTS} "================")
#     MESSAGE(STATUS "Slang Compiler: " ${SLANGC_EXECUTABLE})
#     MESSAGE(STATUS "Shader Sources: " ${SHADER_SOURCES})
#     add_custom_command (
#         OUTPUT ${SHADERS_DIR}/slang.spv
#         COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES}
#                 -target spirv
#                 -profile spirv_1_4
#                 -emit-spirv-directly
#                 -fvk-use-entrypoint-name
#                 ${ENTRY_POINTS}
#                 -o slang.spv
#         WORKING_DIRECTORY ${SHADERS_DIR}
#         DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
#         COMMENT "Compiling Slang Shaders"
#         VERBATIM
#     )
#     add_custom_target (${TARGET}_shaders DEPENDS ${SHADERS_DIR}/slang.spv)
# endfunction()

# set(SHADER_SLANG_SOURCES
#     ${CMAKE_ROOT_SOURCE_DIR}/shaders/shader.slang
# )

# add_slang_shader_target(${PROJECT_NAME} ${SHADER_SLANG_SOURCES})

add_executable(${PROJECT_NAME} src/application/main.cpp)

# add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_shaders)

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
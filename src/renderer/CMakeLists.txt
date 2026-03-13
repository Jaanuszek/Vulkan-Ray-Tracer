cmake_minimum_required(VERSION 3.28)

project(renderer VERSION 1.0.0 DESCRIPTION "Rederer library for Vulkan Ray Tracer")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)

set(IMGUI_SOURCES
    ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui_demo.cpp
    PARENT_SCOPE
)

set(IMGUI_BACKEND
    ${CMAKE_SOURCE_DIR}/third-party/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/third-party/imgui/backends/imgui_impl_vulkan.cpp
    PARENT_SCOPE
)

add_library(${PROJECT_NAME} SHARED
    RTRenderer.cpp
    InstanceManager.cpp
    SwapChainManager.cpp
    Shader.cpp
    CommandBufferManager.cpp
    buffer.cpp
    DeviceManager.cpp
    StorageImage.cpp
    AccelerationStructureManager.cpp
    RayTracingPipeline.cpp
    DescriptorManager.cpp
    Model.cpp
    Image.cpp
    Texture.cpp
    StorageBuffer.cpp
    GUI.cpp
    Scene.cpp
    ${IMGUI_SOURCES}
    ${IMGUI_BACKEND}
)

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME}
    EXPORT_FILE_NAME include/${PROJECT_NAME}_export.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    common
    Core
    cuda_lib
    Vulkan::Vulkan
)

target_precompile_headers(${PROJECT_NAME} REUSE_FROM common)

set_target_properties(${PROJECT_NAME} PROPERTIES 
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER include/RTRenderer.hpp
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "$ORIGIN" # potrzebne zeby renderer widzial cuda_lib xdd
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third-party/tinyobjloader
        ${CMAKE_SOURCE_DIR}/third-party/stb
    PUBLIC
        ${CMAKE_SOURCE_DIR}/third-party/spdlog/include
        ${CMAKE_SOURCE_DIR}/third-party/imgui
        ${CMAKE_SOURCE_DIR}/third-party/imgui/backends
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/lib
    PUBLIC_HEADER DESTINATION ${CMAKE_BINARY_DIR}/include
)
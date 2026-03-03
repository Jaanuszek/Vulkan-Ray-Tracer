cmake_minimum_required(VERSION 3.28)

project(Core VERSION 1.0.0 DESCRIPTION "Dynamic library for Vulkan Ray Tracer")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)

add_library(${PROJECT_NAME} SHARED
    Logger.cpp
    InputManager.cpp
    Camera.cpp
)

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME}
    EXPORT_FILE_NAME include/${PROJECT_NAME}_export.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE common)
target_precompile_headers(${PROJECT_NAME} REUSE_FROM common)

set_target_properties(${PROJECT_NAME} PROPERTIES 
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER include/Logger.hpp
)

target_include_directories(${PROJECT_NAME}
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
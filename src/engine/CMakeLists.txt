cmake_minimum_required(VERSION 3.28)

project(engine VERSION 1.0.0 DESCRIPTION "Engine facade for Vulkan Ray Tracer")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} SHARED
	Engine.cpp
    CustomModels.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
	renderer
	Core
	common
    cuda_lib
)

set_target_properties(${PROJECT_NAME} PROPERTIES 
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER include/Engine.hpp
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "$ORIGIN"
)

target_include_directories(${PROJECT_NAME}
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/renderer/include>
		$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/core/include>
)

install(TARGETS ${PROJECT_NAME}
	LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/lib
	PUBLIC_HEADER DESTINATION ${CMAKE_BINARY_DIR}/include
)

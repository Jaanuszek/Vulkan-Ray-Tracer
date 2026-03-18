#  Automatic library (static or shared)
SET(COMMON_LIBRARY_NAME common)

add_library(${COMMON_LIBRARY_NAME} 
    pch.cpp
    VmaUsage.cpp
    defines.cpp
    CommonStructs.cpp
)

target_precompile_headers(${COMMON_LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/pch.h)
target_include_directories(${COMMON_LIBRARY_NAME} 
    PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third-party/stb
    )
target_compile_options(${COMMON_LIBRARY_NAME} PRIVATE -fPIC)
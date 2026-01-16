#!/bin/bash

# Script to compile GLSL ray tracing shaders to SPIR-V
# Requires glslangValidator or glslc to be installed

SHADER_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Check if glslc is available, otherwise try glslangValidator
if command -v glslc &> /dev/null; then
    COMPILER="glslc"
    echo "Using glslc compiler"
elif command -v glslangValidator &> /dev/null; then
    COMPILER="glslangValidator"
    echo "Using glslangValidator compiler"
else
    echo "Error: Neither glslc nor glslangValidator found. Please install Vulkan SDK."
    exit 1
fi

# Compile function
compile_shader() {
    local input_file=$1
    local output_file=$2
    local stage=$3
    
    echo "Compiling $input_file -> $output_file"
    
    if [ "$COMPILER" == "glslc" ]; then
        glslc -fshader-stage="$stage" "$input_file" -o "$output_file" \
            --target-env=vulkan1.2 \
            -O
    else
        glslangValidator -V "$input_file" -o "$output_file" \
            --target-env vulkan1.2 \
            -S "$stage"
    fi
    
    if [ $? -eq 0 ]; then
        echo "✓ Successfully compiled $output_file"
    else
        echo "✗ Failed to compile $input_file"
        exit 1
    fi
}

# Compile all shaders
compile_shader "$SHADER_DIR/rayGen.rgen" "$SHADER_DIR/raygen.spv" "rgen"
compile_shader "$SHADER_DIR/miss.rmiss" "$SHADER_DIR/miss.spv" "rmiss"
compile_shader "$SHADER_DIR/closesthit.rchit" "$SHADER_DIR/closesthit.spv" "rchit"

echo ""
echo "All shaders compiled successfully!"

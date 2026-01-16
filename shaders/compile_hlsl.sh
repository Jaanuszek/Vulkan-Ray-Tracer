#!/bin/bash

if [ -z "${VULKAN_SDK}" ]; then
    echo "VULKAN_SDK is not set. Please set it to your Vulkan SDK path."
    exit 1
fi

SHADER_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

DXC_PATH="$VULKAN_SDK/bin/dxc"
if [ ! -f "$DXC_PATH" ]; then
    echo "dxc compiler not found in VULKAN_SDK path: $DXC_PATH"
    exit 1
fi

compile_shader() {
    local input_file=$1
    local output_file=$2
    local stage=$3
    local entry_point="${4:-main}"
    
    echo "Compiling $input_file -> $output_file"
    
    "$DXC_PATH" -T lib_6_4 \
    -E ${entry_point} \
    -fspv-target-env=vulkan1.2 \
    "$input_file" \
    -Fo "$output_file" \
    -spirv \
    -fvk-use-scalar-layout \

    if [ $? -eq 0 ]; then
        echo "✓ Successfully compiled $output_file"
    else
        echo "✗ Failed to compile $input_file"
        exit 1
    fi
}

compile_shader "$SHADER_DIR/rayGen.hlsl" "$SHADER_DIR/raygen.spv" "rgen"
compile_shader "$SHADER_DIR/miss.hlsl" "$SHADER_DIR/miss.spv" "rmiss"
compile_shader "$SHADER_DIR/closesthit.hlsl" "$SHADER_DIR/closesthit.spv" "rchit"
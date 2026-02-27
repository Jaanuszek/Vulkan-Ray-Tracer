#!/bin/bash

set -e

ROOT_SHADER_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/.."
SLANG_SHADER_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if command -v slangc >/dev/null 2>&1; then
    COMPILER="slangc"
    echo "Using slangc compiler"
else
    echo "Error: slangc not found. Please install Slang compiler."
    exit 1
fi

compile_shader() {
    local input_file=$1
    local output_file=$2
    local stage=$3

    echo "Compiling $input_file -> $output_file"

    # w razie gdybym chcial miec mixed shader entry points, czyli 
    # rayGenShader, missShader, closestHitShader w jednym pliku .slang
    # to trzeba ustawic:
    # -fvk-use-entrypoint-name
    # -emit-spirv-directly

    # slangc "$input_file" -o "$output_file" -target spirv -entry "$stage" -fvk-use-entrypoint-name -emit-spirv-directly -g2
    slangc "$input_file" -o "$output_file" -target spirv -fvk-use-scalar-layout -fvk-use-entrypoint-name -emit-spirv-directly -g2
}

# compile_shader "${SLANG_SHADER_DIR}/basicShader.slang" "${ROOT_SHADER_DIR}/raygen.spv" "rayGenShader"
# compile_shader "${SLANG_SHADER_DIR}/basicShader.slang" "${ROOT_SHADER_DIR}/miss.spv" "missShader"
# compile_shader "${SLANG_SHADER_DIR}/basicShader.slang" "${ROOT_SHADER_DIR}/closesthit.spv" "closestHitShader"

compile_shader "${SLANG_SHADER_DIR}/basicShader.slang" "${ROOT_SHADER_DIR}/slangTest.spv" "closestHitShader"

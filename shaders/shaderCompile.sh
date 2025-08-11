#! /bin/bash

set -e

if [ ! -f ${VULKAN_SDK} ]; then
    # Expecting the Vulkan SDK is installed in ~/vulkan directory:

    VULKAN_SDK_VERSION=$(find ${HOME}/vulkan/ -depth -maxdepth 1 -type d | xargs basename)
    VULKAN_SDK="${HOME}/vulkan/${VULKAN_SDK_VERSION}/x86_64"
fi

${VULKAN_SDK}/bin/slangc shader.slang -target spirv -profile spirv_1_4\
    -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry fragMain -o slang.spv
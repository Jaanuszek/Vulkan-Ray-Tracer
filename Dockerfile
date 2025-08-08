# syntax=docker/dockerfile:1

FROM ubuntu:latest

ENV rootpath=/app
WORKDIR ${rootpath}

USER root

RUN apt-get update && \
    apt-get install -y python3 python3-pip python3-venv \
    git cmake libglfw3-dev pkg-config libxkbcommon-dev \
    libxinerama-dev libxcursor-dev libxi-dev libxext-dev

# Vulkan download
RUN apt-get install -y libglm-dev libxcb-dri3-0 libxcb-present0 libpciaccess0 \
    libpng-dev libxcb-keysyms1-dev libxcb-dri3-dev libx11-dev g++ gcc \
    libwayland-dev libxrandr-dev libxcb-randr0-dev libxcb-ewmh-dev \
    python-is-python3 bison libx11-xcb-dev liblz4-dev libzstd-dev \
    ocaml-core ninja-build libxml2-dev wayland-protocols python3-jsonschema \
    clang-format qtbase5-dev qt6-base-dev

COPY vulkansdk-linux-* /tmp/

RUN if ls -d /tmp/vulkansdk-linux-* 1> /dev/null 2>&1; then \
        echo "VulkanSDK Tarball exists!"; \
    else \
        echo "VulkanSDK Tarball does not exist!"; \
        exit 1; \
    fi

RUN mkdir -p ~/vulkan && \
    tar xf /tmp/vulkansdk-linux-* -C ~/vulkan && \
    rm /tmp/vulkansdk-linux-*

RUN apt install -y libxcb-xinput0 libxcb-xinerama0 libxcb-cursor-dev

# this is not needed for building, but I will leave it so VAR ENVS are set inside container
RUN VERSION=$(find ~/vulkan -maxdepth 1 -type d | grep "1\." | xargs basename) && \
    echo "Vulkan version: $VERSION" && \
    echo "if [ -d ~/vulkan/$VERSION/x86_64 ]; then" >> ~/.bashrc && \
    echo "    source $HOME/vulkan/$VERSION/setup-env.sh" >> ~/.bashrc && \
    echo "fi" >> ~/.bashrc

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT [ "./entrypoint.sh" ]
CMD [ "bash" ]
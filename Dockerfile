# syntax=docker/dockerfile:1

FROM nvidia/cuda:13.1.2-cudnn-devel-ubuntu24.04

ENV rootpath=/app

WORKDIR ${rootpath}

USER root

RUN apt-get update && \
    apt-get install -y python3 python3-pip python3-venv \
    git cmake libglfw3-dev pkg-config libglfw3-dev

# Vulkan download
RUN apt-get install -y libglm-dev libxcb-dri3-0 libxcb-present0 libpciaccess0 \
    libpng-dev libxcb-keysyms1-dev libxcb-dri3-dev libx11-dev g++ gcc \
    libwayland-dev libxrandr-dev libxcb-randr0-dev libxcb-ewmh-dev \
    python-is-python3 bison libx11-xcb-dev liblz4-dev libzstd-dev \
    ocaml-core ninja-build libxml2-dev wayland-protocols python3-jsonschema \
    clang-format qtbase5-dev qt6-base-dev

RUN apt update && apt install -y wget gnupg && rm -rf /var/lib/apt/lists/*

# Vulkan SDK download
RUN wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | tee /etc/apt/trusted.gpg.d/lunarg.asc
RUN wget -qO /etc/apt/sources.list.d/lunarg-vulkan-noble.list http://packages.lunarg.com/vulkan/lunarg-vulkan-noble.list  
RUN apt update
RUN apt -y install vulkan-sdk && rm -rf /var/lib/apt/lists/*

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT [ "./entrypoint.sh" ]
CMD [ "bash" ]
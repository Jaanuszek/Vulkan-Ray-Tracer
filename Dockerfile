# syntax=docker/dockerfile:1

FROM ubuntu:latest

ENV rootpath=/app
WORKDIR ${rootpath}

USER root

RUN apt-get update && \
    apt-get install -y python3 python3-pip python3-venv \
    git cmake vulkan-tools libvulkan-dev \
    vulkan-utility-libraries-dev spirv-tools \
    libglfw3-dev libglm-dev glslang-tools glslc \ 
    pkg-config

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT [ "./entrypoint.sh" ]
CMD [ "bash" ]
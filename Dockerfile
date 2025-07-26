# syntax=docker/dockerfile:1

FROM ubuntu:latest

ENV rootpath=/app
WORKDIR ${rootpath}

USER root

RUN chown -R $(id -u):$(id -g) ${rootpath}

RUN apt-get update && \
    apt-get install -y python3 python3-pip python3-venv \
    git cmake vulkan-tools libvulkan-dev \
    vulkan-utility-libraries-dev spirv-tools \
    libglfw3-dev libglm-dev pkg-config

# copy everything for now, unitl i came up with some better solution
COPY . ./

RUN chmod +x ./entrypoint.sh

ENTRYPOINT [ "./entrypoint.sh" ]
CMD [ "bash" ]
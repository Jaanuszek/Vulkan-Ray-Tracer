# syntax=docker/dockerfile:1

FROM ubuntu:latest

ENV rootpath=/app
WORKDIR ${rootpath}

USER root

RUN apt-get update && \
    apt-get install -y python3 python3-pip python3-venv && \
    apt-get install -y git cmake
    
# copy everything for now, unitl i came up with some better solution
COPY . ./

RUN chmod +x ./entrypoint.sh

ENTRYPOINT [ "./entrypoint.sh" ]
CMD [ "bash" ]
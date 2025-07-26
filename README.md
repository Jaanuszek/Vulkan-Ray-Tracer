# Vulkan-Ray-Tracer
3D graphics engine (Ray tracing + Radiosity). Written in C++, Vulkan and Cuda.

## Docker
* To build a docker container
    ```bash
    docker build -t test:latest .
    ```

* To run a docker conatiner with bash terminal
    ```bash
    docker run --hostname AppBuilder -it -v $(pwd)/build:/app/build --rm test:latest
    ```

* To run a docker container that builds the app and then exit

    ```bash
    docker run --hostname AppBuilder -it -v $(pwd)/:/app/ --rm test:latest build [-v | --vebose] [-d | --debug]
    ```

* To run a docker container with a custom entrypoint exec
    ```bash
    docker run --entrypoint /bin/bash -it --hostname AppBuilder --rm -v $(pwd)/build:/app/buildtest:latest
    ```
# Vulkan-Ray-Tracer
3D graphics engine (Ray tracing + Radiosity). Written in C++, Vulkan and Cuda.

## prerequisites
- **VULKAN** - [latest](https://vulkan.lunarg.com/sdk/home#linux) 

- **GLFW3**
    ```bash
    apt-get install libglfw3-dev
    ```
## Cloning

This repo uses `submodules`, so make sure to clone it using:
```bash
git clone --recurse-submodules <URL>
```
Or if you already cloned it, just use this command:
```bash
git submodule update --init --recursive
```

## Validation layer

To run application with validation layers on `Ubuntu`, build it in debug mode, and make sure you have installed it on your OS:

```bash
sudo apt install vulkan-validationlayers
```

## Docker

> [!IMPORTANT]
> For this moment (09.08.2025) It is still possible to build the app using docker, but when project 
> will get more complicated, there is a high chance that it will not be possible.


To build a docker container
```bash
docker build -t test:latest .
```

Every container mounted the `build/` and `venv/` folders as `read-write`, and any other folders as `read-only`, so docker container can't delete anything beside `build/` and `venv/` content.

### Building container using `docker compose`

```bash
docker compose run --rm appbuilder_cuda [build | rebuild | test] [ -v | --verbose ] [ -d | --debug ] 
```

#### Building options:
- Without any flags - open docker container, so the user can build the project inside virtual environment
- `build` - Open container, build the project automatically and leave
- `rebuild` - Open container, rebuild existing binary and leave
- `test` - Open container, run unit tests and leave **TODO!!**
### Building container using `docker run`
* To run a docker conatiner with bash terminal
```bash
docker run --hostname AppBuilder -it -v $(pwd)/build:/app/build:rw -v $(pwd)/venv:/app/venv:rw -v $(pwd)/:/app/:ro --rm test:latest
```

* To run a docker container that *builds* the app and then exit

```bash
docker run --hostname AppBuilder -it -v $(pwd)/build:/app/build:rw -v $(pwd)/venv:/app/venv:rw -v $(pwd)/:/app/:ro --rm test:latest build [-v | --vebose] [-d | --debug]
```

* To run a docker container that *rebuilds* the app and then exit

```bash
docker run --hostname AppBuilder -it -v $(pwd)/build:/app/build:rw -v $(pwd)/venv:/app/venv:rw -v $(pwd)/:/app/:ro --rm test:latest rebuild [-v | --vebose] [-d | --debug]
```

* To run a docker container with a custom entrypoint exec
```bash
docker run --hostname AppBuilder -it -v $(pwd)/build:/app/build:rw -v $(pwd)/venv:/app/venv:rw -v $(pwd)/:/app/:ro --rm --entrypoint /bin/bash test:latest
```

### Compiling HLSL Shaders

```bash
~/vulkan/1.4.321.1/x86_64/bin/dxc   -T lib_6_4  -E main -fspv-target-env=vulkan1.1spirv1.4  shaders/rayGen.hlsl   -Fo shaders/raygen.spv   -spirv 
```

```bash
~/vulkan/1.4.321.1/x86_64/bin/dxc   -T lib_6_4  -E main -fspv-target-env=vulkan1.1spirv1.4  shaders/rayGen.hlsl   -Fo shaders/raygen.spv   -spirv -fvk-use-scalar-layout
```

```bash
~/vulkan/1.4.321.1/x86_64/bin/dxc   -T lib_6_4  -E main -fspv-target-env=vulkan1.1spirv1.4  shaders/rayGen.hlsl   -Fo shaders/raygen.spv   -spirv -fvk-use-scalar-layout && ~/vulkan/1.4.321.1/x86_64/bin/dxc   -T lib_6_4  -E main -fspv-target-env=vulkan1.1spirv1.4  shaders/miss.hlsl   -Fo shaders/miss.spv   -spirv -fvk-use-scalar-layout && ~/vulkan/1.4.321.1/x86_64/bin/dxc   -T lib_6_4  -E main -fspv-target-env=vulkan1.1spirv1.4  shaders/closesthit.hlsl   -Fo shaders/closesthit.spv   -spirv -fvk-use-scalar-layout
```

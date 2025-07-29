# Vulkan-Ray-Tracer
3D graphics engine (Ray tracing + Radiosity). Written in C++, Vulkan and Cuda.

## Docker
* To build a docker container
```bash
docker build -t test:latest .
```

Every container has mounted the `build/` folder as `read-write`, and any other folder as `read-only`, so docker container can't delete anything beside `build/` content.

> [!NOTE]
> TODO think about docker-compose!!!

* To run a docker conatiner with bash terminal
```bash
docker run --hostname AppBuilder -it -v $(pwd)/build:/app/build:rw -v $(pwd)/:/app/:ro --rm test:latest
```

* To run a docker container that *builds* the app and then exit

```bash
docker run --hostname AppBuilder -it -v $(pwd)/build:/app/build:rw -v $(pwd)/:/app/:ro --rm test:latest build [-v | --vebose] [-d | --debug]
```

* To run a docker container that *rebuilds* the app and then exit

```bash
docker run --hostname AppBuilder -it -v $(pwd)/build:/app/build:rw -v $(pwd)/:/app/:ro --rm test:latest rebuild [-v | --vebose] [-d | --debug]
```

* To run a docker container with a custom entrypoint exec
* 
```bash
docker run --hostname AppBuilder -it -v $(pwd)/build:/app/build:rw -v $(pwd)/:/app/:ro --rm --entrypoint /bin/bash test:latest
```
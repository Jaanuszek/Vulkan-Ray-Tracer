#!/usr/bin/env python

import argparse
import os
import subprocess
import shutil

# Argument settings
optional_args = argparse.ArgumentParser(add_help=False)
optional_args.add_argument("-d", "--debug", help="Enable debug mode", action="store_true")
optional_args.add_argument("-v", "--verbose", help="Enable verbose output", action="store_true")

parser = argparse.ArgumentParser("Build or test the application")
subparser = parser.add_subparsers(required=True, dest="command")

build_parser = subparser.add_parser("build", parents=[optional_args], help="Build the project")
test_parser = subparser.add_parser("test", parents=[optional_args], help="Run unit tests")
rebuild_parser = subparser.add_parser("rebuild", parents=[optional_args], help="Rebuild the project")

args = parser.parse_args()

# Variables setup, based on provided arguments
build_type:str = "Debug" if args.debug else "Release"
verbose:bool = args.verbose

NVCC_PATH:str = subprocess.run(["which", "nvcc"], capture_output=True, text=True).stdout.strip()
if not NVCC_PATH:
    print("Error: nvcc not found in PATH. Please ensure CUDA is installed and nvcc is accessible.")
    exit(1)

# Delete files in build directory if it exists
build_dir:str = "build"
if not args.command == "rebuild" and os.path.exists(build_dir):
    for filename in os.listdir(build_dir):
        file_path = os.path.join(build_dir, filename)
        try:
            if os.path.isfile(file_path):
                os.remove(file_path)
            elif os.path.isdir(file_path):
                shutil.rmtree(file_path)
        except Exception as e:
            print(f"Error deleting {file_path}: {e}")

os.makedirs(build_dir, exist_ok=True)

# check=True tells that subprocess should raise an exception on error, that's why I implemented a try-catch here
try:
    cmake_conf_args:list = [
        f"-DCMAKE_BUILD_TYPE={build_type}",
        f"-DCMAKE_CUDA_COMPILER={NVCC_PATH}"
    ]
    cmake_build_args:list = []
    if verbose:
        cmake_conf_args.append("-DCMAKE_VERBOSE_MAKEFILE=ON")
        cmake_build_args.append("--verbose")

    subprocess.run(["cmake", ".."] + cmake_conf_args, cwd=build_dir, check=True)
    # subprocess.run(["cmake", "--build", "."] + cmake_build_args, cwd=build_dir, check=True)
    subprocess.run(["cmake", "--build", ".", "-j", "4"] + cmake_build_args, cwd=build_dir, check=True)
    subprocess.run(["cmake", "--install", "."], cwd=build_dir, check=True)
except subprocess.CalledProcessError as e:
    print(f"Error during build: {e}")

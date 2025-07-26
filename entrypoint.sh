#!/bin/bash

set -e

if [ "$1" == "bash" ]; then
    exec /bin/bash
elif [ "$1" == "build" ]; then
    python3 -m venv ./venv
    source ./venv/bin/activate
    pip install -r requirements.txt
    ./build.py
else
    echo "Unknown command: $1"
    exit 1
fi
#!/bin/bash

set -e

pythonVenvDir="/app/venv"

activate_venv_and_install_requirements(){
    if [ ! -d "$pythonVenvDir" ]; then
        python3 -m venv "$pythonVenvDir"
    fi
    source ${pythonVenvDir}/bin/activate
    pip install -r ./requirements.txt
}

command="$1"
shift # Shift args to right, so $1 becomes $2
additionalFlags=()

# $# - number of arguments
while [ $# -gt 0 ]; do
    case "$1" in
        -v|--verbose)
            additionalFlags+=("$1")
            ;;
        -d|--debug)
            additionalFlags+=("$1")
            ;;
        *)
            echo "Unknown flag: $1"
            exit 1
            ;;
    esac
    shift
done

# ${additionalFlags[@]} - all stored in array arguments
echo "Running command: ${command} with flags: ${additionalFlags[@]}"

if [ "${command}" == "bash" ]; then
    exec /bin/bash
elif [ "${command}" == "build" ]; then
    activate_venv_and_install_requirements
    ./build.py build "${additionalFlags[@]}"
elif [ "${command}" == "test" ]; then
    activate_venv_and_install_requirements
    ./build.py test "${additionalFlags[@]}"
else
    echo "Unknown command: $1"
    exit 1
fi
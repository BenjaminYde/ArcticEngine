#!/bin/bash

SCRIPT_DIR=$(dirname $0)
REPO_DIR=$(realpath $CURRENT_DIR/../..)

# allow x11 in docker to forward gui to host
xhost +local:docker
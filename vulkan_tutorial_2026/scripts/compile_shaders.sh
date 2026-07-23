#!/usr/bin/env bash

CURRENT_DIR=$(realpath $(dirname $0))
PROJECT_DIR=$CURRENT_DIR/..

SHADER_DIR=$PROJECT_DIR/shaders

slangc $SHADER_DIR/shader.slang \
    -target spirv  \
    -profile spirv_1_4 \
    -emit-spirv-directly \
    -fvk-use-entrypoint-name \
    -entry vertMain \
    -entry fragMain \
    -o $SHADER_DIR/shader.spv
#!/usr/bin/env bash

/opt/shader-slang-bin/bin/slangc -target spirv -emit-spirv-directly -fvk-use-entrypoint-name -source-embed-style u32 -source-embed-name triangle_spv -o shaders/bin/triangle.h $(dirname "$0")/src/triangle.slang
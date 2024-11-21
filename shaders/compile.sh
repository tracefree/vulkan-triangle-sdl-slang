#!/usr/bin/env bash

/opt/shader-slang-bin/bin/slangc -entry vertex -o $(dirname "$0")/bin/triangle_vert.spv $(dirname "$0")/src/triangle.slang
/opt/shader-slang-bin/bin/slangc -entry fragment -o $(dirname "$0")/bin/triangle_frag.spv $(dirname "$0")/src/triangle.slang
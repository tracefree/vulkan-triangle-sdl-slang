#!/usr/bin/env bash

/opt/shader-slang-bin/bin/slangc -fvk-use-entrypoint-name -o $(dirname "$0")/bin/triangle.spv $(dirname "$0")/src/triangle.slang
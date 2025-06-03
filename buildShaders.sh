#!/bin/bash

prefix=./engine/renderer/vulkan/shaders/

mkdir -p bin/Assets

glslc ${prefix}/shaders.vert -o ./bin/Assets/vert.spv
glslc ${prefix}/shaders.frag -o ./bin/Assets/frag.spv

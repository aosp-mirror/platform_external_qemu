#!/bin/bash
# For debugging emulator Vulkan driver on the host side.
BINDIR=objs
LAYERDIR=$BINDIR/testlib64/layers
export LD_LIBRARY_PATH=$LAYERDIR
export VK_LAYER_PATH=$LAYERDIR
export VK_INSTANCE_LAYERS=\
VK_LAYER_KHRONOS_validation:\
VK_LAYER_LUNARG_api_dump

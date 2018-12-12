#!/bin/bash
# For debugging emulator Vulkan driver on the host side.
BINDIR=objs
LAYERDIR=$BINDIR/testlib64/layers
export LD_LIBRARY_PATH=$LAYERDIR
export VK_LAYER_PATH=$LAYERDIR
export VK_INSTANCE_LAYERS=\
VK_LAYER_GOOGLE_threading:\
VK_LAYER_LUNARG_parameter_validation:\
VK_LAYER_LUNARG_device_limits:\
VK_LAYER_LUNARG_object_tracker:\
VK_LAYER_LUNARG_core_validation:\
VK_LAYER_GOOGLE_unique_objects:\
VK_LAYER_LUNARG_api_dump

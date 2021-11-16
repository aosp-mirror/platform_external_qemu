#!/bin/sh

# Copyright 2020 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Regenerate static translator headers for EGL/GLES1/GLES2,3."

ENTRIES_DIR=android/android-emugl/host/libs/libOpenGLESDispatch
HEADER_OUT_DIR=android/android-emugl/host/include/OpenGLESDispatch
IMPL_OUT_DIR=android/android-emugl/host/libs/libOpenGLESDispatch

function gen_entries_with_mode() {
    local mode=$1
    local entries_file=$2
    local output_path=$3
    android/scripts/gen-entries.py --mode=$mode $entries_file --output=$output_path
}

function funcargs_header() {
    gen_entries_with_mode funcargs $1 $2
}

function functions_header() {
    gen_entries_with_mode functions $1 $2
}

function static_translator_namespaced_header() {
    gen_entries_with_mode static_translator_namespaced_header $1 $2
}

function static_translator_namespaced_stubs() {
    gen_entries_with_mode static_translator_namespaced_stubs $1 $2
}

functions_header $ENTRIES_DIR/render_egl.entries $HEADER_OUT_DIR/RenderEGL_functions.h
functions_header $ENTRIES_DIR/render_egl_extensions.entries $HEADER_OUT_DIR/RenderEGL_extensions_functions.h
functions_header $ENTRIES_DIR/render_egl_snapshot.entries $HEADER_OUT_DIR/RenderEGL_snapshot_functions.h

funcargs_header $ENTRIES_DIR/gles1_extensions.entries $HEADER_OUT_DIR/gles1_extensions_functions.h
funcargs_header $ENTRIES_DIR/gles1_only.entries $HEADER_OUT_DIR/gles1_only_functions.h
funcargs_header $ENTRIES_DIR/gles2_extensions.entries $HEADER_OUT_DIR/gles2_extensions_functions.h
funcargs_header $ENTRIES_DIR/gles2_only.entries $HEADER_OUT_DIR/gles2_only_functions.h
funcargs_header $ENTRIES_DIR/gles31_only.entries $HEADER_OUT_DIR/gles31_only_functions.h
funcargs_header $ENTRIES_DIR/gles3_extensions.entries $HEADER_OUT_DIR/gles3_extensions_functions.h
funcargs_header $ENTRIES_DIR/gles3_only.entries $HEADER_OUT_DIR/gles3_only_functions.h
funcargs_header $ENTRIES_DIR/gles_common.entries $HEADER_OUT_DIR/gles_common_functions.h
funcargs_header $ENTRIES_DIR/gles_extensions.entries $HEADER_OUT_DIR/gles_extensions_functions.h

static_translator_namespaced_header $ENTRIES_DIR/render_egl.entries $HEADER_OUT_DIR/RenderEGL_static_translator_namespaced_header.h
static_translator_namespaced_header $ENTRIES_DIR/render_egl_extensions.entries $HEADER_OUT_DIR/RenderEGL_extensions_static_translator_namespaced_header.h
static_translator_namespaced_header $ENTRIES_DIR/render_egl_snapshot.entries $HEADER_OUT_DIR/RenderEGL_snapshot_static_translator_namespaced_header.h
static_translator_namespaced_header $ENTRIES_DIR/gles1_extensions.entries $HEADER_OUT_DIR/gles1_extensions_static_translator_namespaced_header.h
static_translator_namespaced_header $ENTRIES_DIR/gles1_only.entries $HEADER_OUT_DIR/gles1_only_static_translator_namespaced_header.h
static_translator_namespaced_header $ENTRIES_DIR/gles2_extensions.entries $HEADER_OUT_DIR/gles2_extensions_static_translator_namespaced_header.h
static_translator_namespaced_header $ENTRIES_DIR/gles2_only.entries $HEADER_OUT_DIR/gles2_only_static_translator_namespaced_header.h
static_translator_namespaced_header $ENTRIES_DIR/gles31_only.entries $HEADER_OUT_DIR/gles31_only_static_translator_namespaced_header.h
static_translator_namespaced_header $ENTRIES_DIR/gles3_extensions.entries $HEADER_OUT_DIR/gles3_extensions_static_translator_namespaced_header.h
static_translator_namespaced_header $ENTRIES_DIR/gles3_only.entries $HEADER_OUT_DIR/gles3_only_static_translator_namespaced_header.h
static_translator_namespaced_header $ENTRIES_DIR/gles_common_for_gles1.entries $HEADER_OUT_DIR/gles_common_for_gles1_static_translator_namespaced_header.h
static_translator_namespaced_header $ENTRIES_DIR/gles_common_for_gles2.entries $HEADER_OUT_DIR/gles_common_for_gles2_static_translator_namespaced_header.h
static_translator_namespaced_header $ENTRIES_DIR/gles_extensions_for_gles1.entries $HEADER_OUT_DIR/gles_extensions_for_gles1_static_translator_namespaced_header.h
static_translator_namespaced_header $ENTRIES_DIR/gles_extensions_for_gles2.entries $HEADER_OUT_DIR/gles_extensions_for_gles2_static_translator_namespaced_header.h

static_translator_namespaced_stubs $ENTRIES_DIR/gles1_stubbed_in_translator_namespace.entries $IMPL_OUT_DIR/gles1_stubbed_in_translator_namespace.cpp
static_translator_namespaced_stubs $ENTRIES_DIR/gles2_stubbed_in_translator_namespace.entries $IMPL_OUT_DIR/gles2_stubbed_in_translator_namespace.cpp

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

PROGNAME=$(basename "$0")

# A little script to regenerate
# android/android-emugl/host/libs/libOpenglRender/vulkan/DecompressionShaders.h
# from the content of one or more .spv files. This is based on
# generate-emulator-icons.sh.

OPT_HELP=

FILES=

die () {
    echo "ERROR: $@"
    exit 1
}

for OPT; do
    case $OPT in
        --help|-h|-\?)
            OPT_HELP=true
            ;;
        -*)
            die "Invalid option $OPT, see --help."
            ;;
        *.spv)
            FILES="$FILES $OPT"
            ;;
        *)
            >&2 echo "WARNING: Ignoring non-spv file: $OPT"
            ;;
    esac
done

if [ "$OPT_HELP"]; then
    cat <<EOF
Usage: $PROGNAME [options] shader1.spv [shader2.spv ...]

Regenerate the content of:

    android/android-emugl/host/libs/libOpenglRender/vulkan/DecompressionShaders.h

from a list of files, which is typically taken from:

    android/android-emugl/host/libs/libOpenglRender/vulkan

You will need the 'xxd' tool in your PATH.

Typical usage is:

   $PROGNAME android/android-emugl/host/libs/libOpenglRender/vulkan/*.spv > android/android-emugl/host/libs/libOpenglRender/vulkan/DecompressionShaders.h

Valid options:
    --help|-h|-?  Print this message.

EOF
    exit 0
fi

if [ -z "$FILES" ]; then
    die "Please give a list of SPV files as arguments. See --help."
fi

XXD=$(which xxd 2>/dev/null || true)
if [ -z "$XXD" ]; then
    die "Could not find 'xxd' tool in your PATH. Please install it!"
fi

cat <<EOF
// Copyright 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// DO NOT EDIT

// Auto-generated on $(date +%Y-%m-%d) with the following command:
//     $0 $@
namespace goldfish_vk {
EOF

for IMAGE in $FILES; do
    IMAGEBASE="$(basename $IMAGE)"
    IMAGE_NAME=$(printf "%s" "$IMAGEBASE" | tr '/' '_' | tr '.' '_')
    printf "static const unsigned char %s[] __attribute__((aligned(4))) = {\n" "$IMAGE_NAME"
    xxd -i < $IMAGE
    printf "};\n\n"
done

cat <<EOF

typedef struct {
    const char*           name;
    const unsigned char*  base;
    size_t                size; // bytes; please convert to uint32's later
} SpvFileEntry;

static const SpvFileEntry  sDecompressionShaderFileEntries[] = {
EOF

for IMAGE in $FILES; do
    IMAGEBASE="$(basename $IMAGE)"
    IMAGE_NAME=$(printf "%s" "$IMAGEBASE" | tr '/' '_' | tr '.' '_')
    cat <<EOF
    {
        "$(basename $IMAGE)",
        $IMAGE_NAME,
        sizeof($IMAGE_NAME)
    },
EOF
done

cat <<EOF
};  //  sDecompressionShaderFileEntries[]

} // namespace goldfish_vk
EOF

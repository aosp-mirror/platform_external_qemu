#!/bin/sh

PROGNAME=$(basename "$0")

# A little script to regenerate android/icons.h from the content of one
# or more files.

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
        *.png)
            FILES="$FILES $OPT"
            ;;
        *)
            >&2 echo "WARNING: Ignoring non-png file: $OPT"
            ;;
    esac
done

if [ "$OPT_HELP"]; then
    cat <<EOF
Usage: $PROGNAME [options] image1.png [image2.png ...]

Regenerate the content of android/icons.h from a list of files, which
is typically taken from images/. You will need the 'xxd' tool in your PATH.

Typical usage is:

   $PROGNAME image/*.png > android/icons.h

Valid options:
    --help|-h|-?  Print this message.

EOF
    exit 0
fi

if [ -z "$FILES" ]; then
    die "Please give a list of PNG files as arguments. See --help."
fi

XXD=$(which xxd 2>/dev/null || true)
if [ -z "$XXD" ]; then
    die "Could not find 'xxd' tool in your PATH. Please install it!"
fi

cat <<EOF
/* Copyright $(date +%Y) The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

// DO NOT EDIT

// Auto-generated on $(date +%Y-%m-%d) with the following command:
//     $0 $@

EOF

for IMAGE in $FILES; do
    IMAGE_NAME=$(printf "%s" "$IMAGE" | tr '/' '_' | tr '.' '_')
    printf "static const unsigned char %s[] = {\n" "$IMAGE_NAME"
    xxd -i < $IMAGE
    printf "};\n\n"
done

cat <<EOF
static const FileEntry  _file_entries[] = {
EOF

for IMAGE in $FILES; do
    IMAGE_NAME=$(printf "%s" "$IMAGE" | tr '/' '_' | tr '.' '_')
    cat <<EOF
    {
        "$(basename $IMAGE)",
        $IMAGE_NAME,
        sizeof($IMAGE_NAME)
    },
EOF
done

cat <<EOF
};  //  _file_entries[]
EOF

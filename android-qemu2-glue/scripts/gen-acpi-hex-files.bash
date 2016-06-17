#!/usr/bin/bash

set -e
export LANG=C
export LC_ALL=C

PROGDIR=$(dirname "$0")

die () {
    echo "ERROR: $@"
    exit 1
}

TOPDIR=$(cd "$PROGDIR"/../.. && pwd -P 2>/dev/null || true)
if [ -z "$TOPDIR" ]; then
    die "Could not find top-level qemu-android source directory"
fi

run () {
    echo "COMMAND: $@"
    "$@"
}

find_program () {
    which $1 2>/dev/null || true
}

if [ -z "$IASL" ]; then
    IASL=$(find_program iasl)
    if [ -z "$IASL" ]; then
        die "'iasl' command-line tool is not installed!"
    fi
fi

if [ -z "$PYTHON" ]; then
    PYTHON=$(find_program python)
    if [ -z "$PYTHON" ]; then
        die "'python' interpreter is not installed!"
    fi
fi

if [ -z "$CPP" ]; then
    CPP=$(find_program cpp)
    if [ -z "$CPP" ]; then
        die "'cpp' pre-processor is not installed!"
    fi
fi

# $1: target file (e.g. hw/i386/acpi-dsdt.hex.generated)
# $2: source file (e.g. hw/i386/acpi-dsdt.dsl)
# $3:
generate_hex () {
    local prefix=${1%%.generated}
    local dst=/tmp/$2
    local dstdir=$(dirname "$dst")
    prefix=${prefix%%.hex}
    mkdir -p $dstdir
    $CPP -x c -P -I $TOPDIR -I $TOPDIR/include -I $TOPDIR/hw/i386 $2 -o $dst.i.orig
    $PYTHON $TOPDIR/scripts/acpi_extract_preprocess.py $dst.i.orig > $dst.i
    $IASL -Pn -vs -l -tc -p $prefix $dst.i
    $PYTHON $TOPDIR/scripts/acpi_extract.py $prefix.lst > $prefix.off
    cat $prefix.off > $1
    rm -f $prefix.aml $prefix.hex $prefix.lst $prefix.off
}

generate_hex hw/i386/acpi-dsdt.hex.generated hw/i386/acpi-dsdt.dsl

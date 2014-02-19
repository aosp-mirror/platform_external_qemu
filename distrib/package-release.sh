#!/bin/bash

# This script is used to rebuild all emulator binaries from sources
# and package them for easier distribution.

set -e
export LANG=C
export LC_ALL=C

PROGDIR=$(dirname "$0")
PROGNAME=$(basename "$0")

panic () {
  echo "ERROR: $@"
  exit 1
}

VERBOSE=1

# Dump a message if VERBOSE is greater or equal to $1
# $1: level
# $2+: Message to print.
dump_n () {
    local LEVEL=$1
    shift
    if [ "$VERBOSE" -ge "$LEVEL" ]; then
        printf "%s\n" "$@"
    fi
}

# Dump a message at VERBOSE level 1 (the default one).
dump () {
    dump_n 1 "$@"
}

# Dump a message at VERBOSE level 2 (if --verbose was used).
log () {
    dump_n 2 "$@"
}

# Run a command, dump its output depending on VERBOSE level, i.e.:
#  0 -> Don't display anything.
#  1 -> Display error messages only.
#  2 -> Display the command, its output and error.
run () {
    case $VERBOSE in
        0)
            "$@" >/dev/null 2>&1
            ;;
        1)
            "$@" >/dev/null
            ;;
        *)
            echo "COMMAND: $@"
            "$@"
            ;;
    esac
}

# $1: Source directory.
# $2: Destination directory.
# $3: List of files to copy, relative to $1 (if empty, all files will be copied).
copy_directory_files () {
  local SRCDIR DSTDIR FILES
  SRCDIR=$1
  DSTDIR=$2
  shift; shift;
  FILES="$@"

  mkdir -p "$DSTDIR" || panic "Cannot create destination directory: $DSTDIR"
  (cd $SRCDIR && tar cf - $FILES) | (cd $DSTDIR && tar xf -)
}

# $1: Source directory (must be a git checkout).
# $2: Destination directory.
copy_directory_git_files () {
  local SRCDIR DSTDIR FILES
  SRCDIR=$1
  DSTDIR=$2
  log "Copying git sources from $SRCDIR to $DSTDIR"
  # The list of names can contain spaces, so put them in a file to avoid
  # any issues.
  TMP_FILE_LIST=$(mktemp)
  (cd $SRCDIR && git ls-files) > $TMP_FILE_LIST
  mkdir -p "$DSTDIR" || panic "Cannot create destination directory: $DSTDIR"
  (cd $SRCDIR && tar cf - -T $TMP_FILE_LIST) | (cd $DSTDIR && tar xf -)
  rm -f $TMP_FILE_LIST
}

# Convert a comma-separated list into a space-separated one.
commas_to_spaces () {
  printf "%s" "$@" | tr ',' ' '
}

# Defaults.
DEFAULT_REVISION=$(date +%Y%m%d)
DEFAULT_PKG_PREFIX=android-emulator
DEFAULT_PKG_DIR=/tmp

case $(uname -s) in
    Linux)
        DEFAULT_SYSTEMS="linux,windows"
        HOST_SYSTEM=linux
        ;;
    Darwin)
        DEFAULT_SYSTEMS="darwin"
        HOST_SYSTEM=darwin
        ;;
    *)
        panic "Unsupported system! This can only run on Linux and Darwin."
esac

# Command-line parsing.
DO_HELP=
OPT_PKG_DIR=
OPT_PKG_PREFIX=
OPT_REVISION=
OPT_SOURCES=
OPT_SYSTEM=

for OPT; do
    case $OPT in
        --help|-?)
            DO_HELP=true
            ;;
        --package-dir=*)
            OPT_PKG_DIR=${OPT##--package-dir=}
            ;;
        --package-prefix=*)
            OPT_PKG_PREFIX=${OPT##--package-prefix=}
            ;;
        --quiet)
            VERBOSE=$(( $VERBOSE - 1 ))
            ;;
        --sources)
            OPT_SOURCES=true
            ;;
        --revision=*)
            OPT_REVISION=${OPT##--revision=}
            ;;
        --system=*)
            OPT_SYSTEM=${OPT##--system=}
            ;;
        --verbose)
            VERBOSE=$(( $VERBOSE + 1 ))
            ;;
        -*)
            panic "Unsupported option '$OPT', see --help."
            ;;
        *)
            panic "Unsupported parameter '$OPT', see --help."
    esac
done

if [ "$DO_HELP" ]; then
    cat <<EOF
Usage: $PROGNAME [options]

Rebuild the emulator binaries from source and package them into tarballs
for easier distribution.

New packages are placed by default at $DEFAULT_PKG_DIR
Use --package-dir=<path> to use another output directory.

Packages names are prefixed with $DEFAULT_PKG_PREFIX-<revision>, where
the <revision> is the current ISO date by default. You can use
--package-prefix=<prefix> and --revision=<revision> to change this.

Binary packages will include the OpenGLES emulation libraries if they can
be found in your current workspace, not otherwise.

Use --sources option to also generate a source tarball.

Valid options (defaults are inside brackets):
    --help | -?           Print this message.
    --package-dir=<path>  Change package output directory [$DEFAULT_PKG_DIR].
    --revision=<name>     Change revision [$DEFAULT_REVISION].
    --sources             Also create sources package.
    --system=<list>       Specify host system list [$DEFAULT_SYSTEMS].

EOF
    exit 0
fi

if [ "$OPT_PKG_PREFIX" ]; then
    PKG_PREFIX=$OPT_PKG_PREFIX
else
    PKG_PREFIX=$DEFAULT_PKG_PREFIX
    log "Auto-config: --package-prefix=$PKG_PREFIX"
fi

if [ "$OPT_REVISION" ]; then
    PKG_REVISION=$OPT_REVISION
else
    PKG_REVISION=$DEFAULT_REVISION
    log "Auto-config: --revision=$PKG_REVISION"
fi

if [ "$OPT_PKG_DIR" ]; then
    PKG_DIR=$OPT_PKG_DIR
    mkdir -p "$PKG_DIR" || panic "Can't create directory: $PKG_DIR"
else
    PKG_DIR=$DEFAULT_PKG_DIR
    log "Auto-config: --package-dir=$PKG_DIR"
fi

if [ "$OPT_SYSTEM" ]; then
    SYSTEMS=$(commas_to_spaces $OPT_SYSTEM)
else
    SYSTEMS=$(commas_to_spaces $DEFAULT_SYSTEMS)
    log "Auto-config: --system=$SYSTEMS"
fi

# Default build directory.
TEMP_BUILD_DIR=/tmp/$USER-qemu-package-binaries

# Ensure the build directory is removed when the script exits or is
# interrupted.
clean_exit () {
    if [ -n "$TEMP_BUILD_DIR" -a -d "$TEMP_BUILD_DIR" ]; then
        rm -rf "$TEMP_BUILD_DIR"
    fi
    exit $?
}

trap "clean_exit 0" EXIT
trap "clean_exit \$?" QUIT HUP INT

# Do some sanity checks to verify that the current source directory
# doesn't have unchecked files and other bad things lingering.

# Assume this script is under distrib/
QEMU_DIR=$(cd "$PROGDIR"/.. && pwd -P)
log "Found emulator directory: $QEMU_DIR"

cd $QEMU_DIR
if [ ! -d "$QEMU_DIR"/.git ]; then
    panic "This directory must be a checkout of \$AOSP/platform/external/qemu!"
fi
UNCHECKED_FILES=$(git ls-files -o -x objs/ -x images/emulator_icon.o)
if [ "$UNCHECKED_FILES" ]; then
    echo "ERROR: There are unchecked files in the current directory!"
    echo "Please remove them:"
    echo "$UNCHECKED_FILES"
    exit 1
fi

GTEST_DIR=$(dirname $QEMU_DIR)/gtest
if [ ! -d "$GTEST_DIR" ]; then
  panic "Cannot find GoogleTest source directory: $GTEST_DIR"
fi
log "Found GoogleTest directory: $GTEST_DIR"

for SYSTEM in $SYSTEMS; do
    PKG_NAME="$PKG_REVISION-$SYSTEM"
    dump "[$PKG_NAME] Rebuilding binaries from sources."
    run cd $QEMU_DIR
    case $SYSTEM in
        $HOST_SYSTEM)
            run ./android-rebuild.sh || panic "Use ./android-rebuild.sh to see why."
            ;;
        windows)
            if [ "$HOST_SYSTEM" != "linux" ]; then
                panic "Windows binaries can only be rebuilt on Linux!"
            fi
            run ./android-rebuild.sh --mingw || panic "Use ./android-rebuild.sh --mingw to see why."
            ;;
        *)
            panic "Can't rebuild $SYSTEM binaries on $HOST_SYSTEM for now!"
            ;;
    esac

    dump "[$PKG_NAME] Copying emulator binaries."
    TEMP_PKG_DIR=$TEMP_BUILD_DIR/$SYSTEM/$PKG_PREFIX-$PKG_REVISION
    run mkdir -p "$TEMP_PKG_DIR"/tools

    run cp -p objs/emulator* "$TEMP_PKG_DIR"/tools
    if [ -d "objs/lib" ]; then
        dump "[$PKG_NAME] Copying GLES emulation libraries."
        run mkdir -p "$TEMP_PKG_DIR"/tools/lib
        run cp -p objs/lib/* "$TEMP_PKG_DIR"/tools/lib/
    fi

    dump "[$PKG_NAME] Creating README file."
    cat > $TEMP_PKG_DIR/README <<EOF
This directory contains Android emulator binaries. You can use them directly
by defining ANDROID_SDK_ROOT in your environment, then call tools/emulator
with the usual set of options.

To install them directly into your SDK, copy them with:

    cp -r tools/* \$ANDROID_SDK_ROOT/tools/
EOF

    dump "[$PKG_NAME] Copying license files."
    mkdir -p "$TEMP_PKG_DIR"/licenses/
    cp COPYING COPYING.LIB "$TEMP_PKG_DIR"/licenses/

    dump "[$PKG_NAME] Creating tarball."
    PKG_FILE=$PKG_DIR/$PKG_PREFIX-$PKG_REVISION-$SYSTEM.tar.bz2
    (run cd "$TEMP_BUILD_DIR"/$SYSTEM && run tar cf $PKG_FILE $PKG_PREFIX-$PKG_REVISION)
done

if [ "$OPT_SOURCES" ]; then
    BUILD_DIR=$TEMP_BUILD_DIR/sources/$PKG_PREFIX-$PKG_REVISION
    PKG_NAME="$PKG_REVISION-sources"
    dump "[$PKG_NAME] Copying GoogleTest source files."
    copy_directory_git_files "$GTEST_DIR" "$BUILD_DIR"/gtest

    dump "[$PKG_NAME] Copying Emulator source files."
    copy_directory_git_files "$QEMU_DIR" "$BUILD_DIR"/qemu

    dump "[$PKG_NAME] Generating README file."
    cat > "$BUILD_DIR"/README <<EOF
This directory contains the sources of the Android emulator.
Use './rebuild.sh' to rebuild the binaries from scratch.
EOF

    dump "[$PKG_NAME] Generating rebuild script."
    cat > "$BUILD_DIR"/rebuild.sh <<EOF
#!/bin/sh

# Auto-generated script used to rebuild the Android emulator binaries
# from sources. Note that this does not include the GLES emulation
# libraries.

cd \$(dirname "\$0") &&
(cd qemu && ./android-rebuild.sh --ignore-audio) &&
mkdir -p bin/ &&
cp -rfp qemu/objs/emulator* bin/ &&
echo "Emulator binaries are under \$(pwd -P)/bin/"
echo "IMPORTANT: The GLES emulation libraries must be copied to:"
echo "    \$(pwd -P)/bin/lib"
EOF

    chmod +x "$BUILD_DIR"/rebuild.sh

    PKG_FILE=$PKG_DIR/$PKG_PREFIX-$PKG_REVISION-sources.tar.bz2
    dump "[$PKG_NAME] Creating tarball..."
    (run cd "$BUILD_DIR"/.. && run tar cjf "$PKG_FILE" $PKG_PREFIX-$PKG_REVISION)
fi

dump "Done. See $PKG_DIR"
ls -l "$PKG_DIR"/$PKG_PREFIX-$PKG_REVISION*

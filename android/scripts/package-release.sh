#!/bin/sh

# Copyright 2015 The Android Open Source Project
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

. $(dirname "$0")/utils/common.shi

shell_import utils/aosp_dir.shi
shell_import utils/aosp_prebuilts_dir.shi
shell_import utils/emulator_prebuilts.shi
shell_import utils/option_parser.shi
shell_import utils/package_builder.shi

###
###  CONFIGURATION DEFAULTS
###

# The list of AOSP directories that contain relevant sources for this
# script.
AOSP_SOURCE_SUBDIRS="external/qemu external/qemu-android external/gtest"

# Default package output directory.
DEFAULT_PKG_DIR=/tmp

# Default package name prefix.
DEFAULT_PKG_PREFIX=android-emulator

# Default package revision name.
DEFAULT_REVISION=$(date +%Y%m%d)

# The list of GPU emulation libraries.
EMUGL_LIBRARIES="OpenglRender EGL_translator GLES_CM_translator GLES_V2_translator"

# The list of Emugl backend directories under $EXEC_DIR/<lib>/
EMUGL_BACKEND_DIRS="gles_mesa"

###
###  UTILITY FUNCTIONS
###

# Get the name of a variable matching a directory name.
# $1: subdirectory name (e.g. 'external/qemu-android')
# Out: variable name (e.g. 'AOSP_COMMIT_external_qemu_android')
_get_aosp_subdir_varname () {
    local __aosp_subdir="$1"
    echo "AOSP_COMMIT_$(printf "%s" "$__aosp_subdir" | tr '/' '_' | tr '-' '_')"
}

# Get the commit corresponding to a given AOSP source sub-directory.
# NOTE: One must call extract_subdir_git_history previously!
# $1: subdirectory name (e.g. 'external/qemu')
# Out: latest git commit for directory.
_get_aosp_subdir_commit () {
    var_value $(_get_aosp_subdir_varname $1)
}

_get_aosp_subdir_commit_description () {
    var_value $(_get_aosp_subdir_varname $1)__TEXT
}

# Extract the git commit SHA1 of a given directory, and put its value
# in a destination variable. If the target directory is not the root
# of a git checkout, abort.
# $1: Destination variable name.
# $2: Git directory.
# Example:   extract_commit_description GTEST_DESC "$GTEST_DIR"
_extract_git_commit_description () {
    local VARNAME GIT_DIR SHA1
    VARNAME=$1
    GIT_DIR=$2
    # Extract the commit description, then escape (') characters in it.
    SHA1=$(cd $GIT_DIR && git log --oneline --no-merges -1 .) || \
        panic "Not a Git directory: $GIT_DIR"

    var_assign ${VARNAME}__TEXT "$SHA1"
    SHA1=$(echo "$SHA1" | awk '{ print $1; }')
    var_assign ${VARNAME} $SHA1
}

# Extract the previous commit of a given AOSP source sub-directory
# from the README file.
# $1: Variable name, which receives the value.
# $2: Path to README file (e.g. $AOSP/prebuilts/android-emulator/README)
# $3: AOSP source sub-directory (e.g. external/qemu)
_extract_previous_git_commit_from_readme () {
    local SHA1
    SHA1=$(awk '$1 == "'$3'" { print $2; }' "$2")
    var_assign $1 "$SHA1"
}

# Extract the previous and current git history of a given AOSP sub-directory.
# One has to call this function before _get_aosp_subdir_commit or
# _get_aosp_subdir_commit_description.
# $1: AOSP sub-directory.
# $2: AOSP root directory.
# $2: Target AOSP prebuilts directory
extract_subdir_git_history () {
    local VARNAME="$(_get_aosp_subdir_varname $1)"
    local SUBDIR="$2/$1"
    local TARGET_PREBUILTS_DIR="$3"
    if [ ! -d "$SUBDIR" ]; then
        panic "Missing required source directory: $SUBDIR"
    fi
    log "Found source directory: $SUBDIR"
    _extract_git_commit_description $VARNAME "$SUBDIR"
    log "Found current $1 commit: $(var_value $VARNAME)"
    # If there is an old prebuilts directory somewhere, read
    # the old README file to extract the previous commits there.
    README_FILE=$TARGET_PREBUILTS_DIR/README
    if [ -f "$README_FILE" ]; then
        _extract_previous_git_commit_from_readme \
            PREV_$VARNAME "$README_FILE" "$1"
        log "Found previous $1 commit: $(var_value PREV_$VARNAME)"
    fi
}

# Copy a file to a destination, creating the output directory if needed.
# $1: Source file path
# $2: Destination file path
copy_file_into () {
    local SRCFILE="$1"
    local DSTDIR="${2%%/}"
    if [ ! -d "$DSTDIR" ]; then
        run mkdir -p "$DSTDIR" ||
                panic "Cannot create destination directory for: $DSTDIR"
    fi
    run cp -p "$SRCFILE" "$DSTDIR/$(basename "$SRCFILE")"
}

# Copy a list of files from one source directory to a destination one.
# $1: Source directory.
# $2: Destination directory.
# $3+: List of files to copy, relative to $1 (if empty, all files will be copied).
copy_directory_files () {
  local SRCDIR DSTDIR FILES
  SRCDIR=$1
  DSTDIR=$2
  shift; shift;
  FILES="$@"

  mkdir -p "$DSTDIR" || panic "Cannot create destination directory: $DSTDIR"
  (cd $SRCDIR && tar cf - $FILES) | (cd $DSTDIR && tar xf -) ||
        panic "Could not copy files to $DSTDIR"
}

# Copy all git sources from one source directory to a destination directory.
# The sources are those listed by 'git ls-files'.
# $1: Source directory (must be a git checkout).
# $2: Destination directory.
copy_directory_git_files () {
  local SRCDIR DSTDIR TMP_FILE_LIST RET
  SRCDIR=$1
  DSTDIR=$2
  log "Copying git sources from $SRCDIR to $DSTDIR"
  # The list of names can contain spaces, so put them in a file to avoid
  # any issues.
  TMP_FILE_LIST=$(mktemp)
  (cd $SRCDIR && git ls-files) > $TMP_FILE_LIST
  mkdir -p "$DSTDIR" || panic "Cannot create destination directory: $DSTDIR"
  RET=0
  (cd $SRCDIR && tar cf - -T $TMP_FILE_LIST) | (cd $DSTDIR && tar xf -) || \
        RET=$?
  rm -f "$TMP_FILE_LIST"
  return $RET
}

# Return a sorted list of all files under a given top-level directory,
# Matching one or more filters.
# $1: top-level directory
# $2+: option file filters. If empty, all files under $1 are taken.
list_files_under () {
    local TOP_DIR="$1"
    shift
    local FILTERS="$*"
    if [ -z "$FILTERS" ]; then
        FILTERS="*"
    fi
    local FILTER FILE FILES TMP_LIST
    TMP_LIST=$(mktemp)
    for FILTER in "$FILTERS"; do
        FILES=$( (cd "$TOP_DIR" && ls -d $FILTER) 2>/dev/null || true)
        for FILE in $FILES; do
            (cd "$TOP_DIR" && find "$FILE" -type f -o -type l 2>/dev/null || true) >> $TMP_LIST
        done
    done
    cat $TMP_LIST | sort -u > $TMP_LIST.tmp
    rm -f $TMP_LIST
    mv $TMP_LIST.tmp $TMP_LIST
    cat "$TMP_LIST"
    rm -f "$TMP_LIST"
}

# Create archive
# $1: Package file
# $2: Source directory
# $3+: List of files/directories to package.
package_archive_files () {
    local PKG_FILE PKG_DIR TMP_FILE_LIST TARFLAGS RET FILTER FILE FILES
    PKG_FILE=$1
    PKG_DIR=$2
    shift; shift;
    # Try to make reproduceable tarballs by forcing the order of files
    # within the archive, as well as setting the user/group names and
    # date to fixed values.
    TMP_FILE_LIST=$(mktemp)
    list_files_under "$PKG_DIR" "$@" > "$TMP_FILE_LIST"
    case $PKG_FILE in
        *.tar)
            TARFLAGS=""
            ;;
        *.tar.gz)
            TARFLAGS=z
            ;;
        *.tar.bz2)
            TARFLAGS=j
            ;;
        *.tar.xz)
            TARFLAGS=J
            ;;
        *)
            panic "Don't know how to create package: $PKG_FILE"
            ;;
    esac
    if [ $(get_verbosity) -gt 2 ]; then
        TARFLAGS=${TARFLAGS}v
    fi

    RET=0
    run tar c${TARFLAGS}f "$PKG_FILE" \
            -C "$PKG_DIR" \
            -T "$TMP_FILE_LIST" \
            --owner=android \
            --group=android \
            --mtime="2015-01-01 00:00:00" \
        || RET=$?
    rm -f "$TMP_FILE_LIST"
    return $RET
}

# Convert a list of hosts to a list of operating systems.
# This also removes duplicates.
# $1: List of host systems (e.g. 'linux-x86_64 darwin-x86_64')
# Out: List of operating systems (e.g. 'linux darwin')
convert_host_list_to_os_list () {
    local RET HOST
    for HOST in "$@"; do
        case $HOST in
            linux-*)
                var_append RET "linux"
                ;;
            darwin-*)
                var_append RET "darwin"
                ;;
            windows-*)
                var_append RET "windows"
                ;;
            *)
                var_append RET "$HOST"
                ;;
        esac
    done
    printf %s "$RET" | tr ' ' '\n' | sort -u | tr '\n' ' '
}

# Return shared library extension for a given host sytem
# $1: host os name (e.g. 'windows' or 'windows-x86_64')
# Out: DLL extension (e.g. '.dll')
dll_ext_for () {
    case $1 in
        windows*)
            printf ".dll"
            ;;
        darwin*)
            printf ".dylib"
            ;;
        *)
            printf ".so"
    esac
}

# Return executable extension for a given host system
# $1: host os name (e.g. 'windows' or 'windows-x86_64')
# Out: exe extension (e.g. '.exe')
exe_ext_for () {
    case $1 in
        windows*)
            printf ".exe"
            ;;
        *)
            printf ""
    esac
}

###
###  COMMAND LINE PARSING
###

OPT_PKG_DIR=
option_register_var "--package-dir=<dir>" OPT_PKG_DIR \
        "Change package output directory [$DEFAULT_PKG_DIR]"

OPT_PKG_PREFIX=
option_register_var "--package-prefix=<prefix>" OPT_PKG_PREFIX \
        "Change package name prefix [$DEFAULT_PKG_PREFIX]"
OPT_REVISION=
option_register_var "--revision=<name>" OPT_REVISION \
        "Change revision [$DEFAULT_REVISION]"

OPT_SOURCES=
option_register_var "--sources" OPT_SOURCES "Also create sources package."

OPT_COPY_PREBUILTS=
option_register_var "--copy-prebuilts=<dir>" OPT_COPY_PREBUILTS \
        "Copy final emulator binaries to <path>/prebuilts/android-emulator"

package_builder_register_options
aosp_prebuilts_dir_register_options
prebuilts_dir_register_option

PROGRAM_PARAMETERS=

PROGRAM_DESCRIPTION=\
"Rebuild the emulator binaries from source and package them into tarballs
for easier distribution.

New packages are placed by default at $DEFAULT_PKG_DIR
Use --package-dir=<path> to use another output directory.

Packages names are prefixed with $DEFAULT_PKG_PREFIX-<revision>, where
the <revision> is the current ISO date by default. You can use
--package-prefix=<prefix> and --revision=<revision> to change this.

Use --sources option to also generate a source tarball.

Use --darwin-ssh=<host> to build perform a remote build of the Darwin
binaries on a remote host through ssh. Note that this forces --sources
as well. You can also define ANDROID_EMULATOR_DARWIN_SSH in your
environment to setup a default value for this option.

Use --copy-prebuilts=<path> to specify the path of an AOSP workspace/checkout,
and to copy 64-bit prebuilt binaries for all 3 host platforms to
<path>/prebuilts/android-emulator/.

This option requires the use of --darwin-ssh=<host> or
ANDROID_EMULATOR_DARWIN_SSH to build the Darwin binaries."

option_parse "$@"

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

package_builder_process_options qemu-package-binaries
aosp_prebuilts_dir_parse_options
prebuilts_dir_parse_option

if [ -z "$OPT_SOURCES" -a "$DARWIN_SSH" ]; then
    OPT_SOURCES=true
    log "Auto-config: --sources  [darwin-ssh]"
fi

TARGET_AOSP=
if [ "$OPT_COPY_PREBUILTS" ]; then
    if [ -z "$DARWIN_SSH" ]; then
        panic "The --copy-prebuilts=<dir> option requires --darwin-ssh=<host>."
    fi
    TARGET_AOSP=$OPT_COPY_PREBUILTS
    if [ ! -d "$TARGET_AOSP/prebuilts/android-emulator" ]; then
        panic "Could not find prebuilts/android-emulator in: '$TARGET_AOSP'"
    fi
    TARGET_PREBUILTS_DIR=$TARGET_AOSP/prebuilts/android-emulator
    log "Using AOSP prebuilts directory: $TARGET_PREBUILTS_DIR"
    mkdir -p "$TARGET_PREBUILTS_DIR"
elif [ -f "$(program_directory)/../../../../build/envsetup.sh" ]; then
    TARGET_AOSP=$(cd $(program_directory)/../../../.. && pwd -P)
    log "Found AOSP checkout directory: $TARGET_AOSP"
    TARGET_PREBUILTS_DIR=$TARGET_AOSP/prebuilts/android-emulator
    if [ ! -d "$TARGET_PREBUILTS_DIR" ]; then
        TARGET_PREBUILTS_DIR=
    fi
else
    TARGET_AOSP=$(cd $(program_directory)/../../../.. && pwd -P)
fi

###
###  DO THE WORK
###

REBUILD_FLAGS="--verbosity=$(get_verbosity)"
if [ "$OPT_DEBUG" ]; then
    var_append REBUILD_FLAGS "--debug"
fi

log "Building for the following systems: $HOST_SYSTEMS"

# Assume this script is under android/scripts/
QEMU_DIR=$(cd $(program_directory)/../.. && pwd -P)
log "Found emulator directory: $QEMU_DIR"

cd $QEMU_DIR
if [ ! -d "$QEMU_DIR"/.git ]; then
    panic "This directory must be a checkout of \$AOSP/platform/external/qemu!"
fi
UNCHECKED_FILES=$(git ls-files -o -x objs/ -x images/emulator_icon32.o -x images/emulator_icon64.o)
if [ "$UNCHECKED_FILES" ]; then
    dump "ERROR: There are unchecked files in the current directory!"
    dump "Please remove them:"
    dump "$UNCHECKED_FILES"
    exit 1
fi

for AOSP_SUBDIR in $AOSP_SOURCE_SUBDIRS; do
    extract_subdir_git_history \
            $AOSP_SUBDIR \
            "$QEMU_DIR"/../.. \
            "$TARGET_PREBUILTS_DIR"
done

###
###  CREATE SOURCES PACKAGE IS NEEDED
###

SOURCES_PKG_FILE=
if [ "$OPT_SOURCES" ]; then
    BUILD_DIR=$TEMP_DIR/sources/$PKG_PREFIX-$PKG_REVISION
    PKG_NAME="$PKG_REVISION-sources"
    for AOSP_SUBDIR in $AOSP_SOURCE_SUBDIRS; do
        dump "[$PKG_NAME] Copying $AOSP_SUBDIR source files."
        copy_directory_git_files "$QEMU_DIR/../../$AOSP_SUBDIR" "$BUILD_DIR"/$(basename $AOSP_SUBDIR)
    done

    dump "[$PKG_NAME] Generating README file."
    cat > "$BUILD_DIR"/README <<EOF
This directory contains the sources of the Android emulator.
Use './rebuild.sh' to rebuild the binaries from scratch.
EOF

    dump "[$PKG_NAME] Generating rebuild script."
    cat > "$BUILD_DIR"/rebuild.sh <<EOF
#!/bin/sh

# Auto-generated script used to rebuild the Android emulator binaries
# from sources.

cd \$(dirname "\$0") &&
(cd qemu && ./android-rebuild.sh --ignore-audio) &&
mkdir -p bin/ &&
cp -rfp qemu/objs/emulator* bin/ &&
echo "Emulator binaries are under \$(pwd -P)/bin/"
EOF

    chmod +x "$BUILD_DIR"/rebuild.sh

    PKG_FILE=$PKG_DIR/$PKG_PREFIX-$PKG_REVISION-sources.tar.bz2
    SOURCES_PKG_FILE=$PKG_FILE
    dump "[$PKG_NAME] Creating tarball..."
    package_archive_files "$PKG_FILE" "$BUILD_DIR"/.. $PKG_PREFIX-$PKG_REVISION
fi

###
###  PERFORM REMOTE DARWIN BUILD IF NEEDED
###

# $1: System (os) name.
create_binaries_package () {
    local SYSTEM=$1
    local PKG_NAME=$PKG_REVISION-$SYSTEM
    local DLLEXT=$(dll_ext_for $SYSTEM)
    local EXEEXT=$(exe_ext_for $SYSTEM)
    local FILE LIB
    dump "[$PKG_NAME] Copying emulator binaries."
    TEMP_PKG_DIR=$TEMP_DIR/$SYSTEM/$PKG_PREFIX-$PKG_REVISION

    FILE=emulator$EXEEXT
    copy_file_into objs/$FILE "$TEMP_PKG_DIR"/tools/
    if [ -d "objs/lib" ]; then
        for FILE in objs/emulator-*; do
            copy_file_into $FILE "$TEMP_PKG_DIR"/tools/
        done
        dump "[$PKG_NAME] Copying 32-bit GLES emulation libraries."
        for LIB in $EMUGL_LIBRARIES; do
            copy_file_into objs/lib/lib$LIB$DLLEXT \
                           "$TEMP_PKG_DIR"/tools/lib/
        done
    fi
    if [ -d "objs/lib64" ]; then
        for FILE in objs/emulator64-*; do
            copy_file_into $FILE "$TEMP_PKG_DIR"/tools/
        done
        dump "[$PKG_NAME] Copying 64-bit GLES emulation libraries."
        for LIB in $EMUGL_LIBRARIES; do
            copy_file_into objs/lib64/lib64$LIB$DLLEXT \
                           "$TEMP_PKG_DIR"/tools/lib64/
        done
    fi
    if [ -d "objs/lib/pc-bios" ]; then
        dump "[$PKG_NAME] Copying x86 PC BIOS ROM files."
        copy_directory_files objs/lib/pc-bios \
                "$TEMP_PKG_DIR"/tools/lib/pc-bios "*.bin"
    fi
    if [ -d "objs/qemu" ]; then
        QEMU_BINARIES=$(list_files_under objs/qemu "$SYSTEM-*/qemu-system-*")
        if [ "$QEMU_BINARIES" ]; then
            for QEMU_BINARY in $QEMU_BINARIES; do
                dump "[$PKG_NAME] Copying $QEMU_BINARY."
                copy_file_into \
                        objs/qemu/$QEMU_BINARY \
                        "$TEMP_PKG_DIR"/tools/qemu/$(dirname $QEMU_BINARY)
            done
        fi
    fi

    # Copy Emugl backend directories, if any.
    for EMUGL_BACKEND in $EMUGL_BACKEND_DIRS; do
        MESA_LIBDIR=objs/lib/$EMUGL_BACKEND
        if [ -d "$MESA_LIBDIR" ]; then
            MESA_BINARIES=$(list_files_under "$MESA_LIBDIR" "")
            if [ "$MESA_BINARIES" ] ;then
                dump "[$PKG_NAME] Copying $EMUGL_BACKEND 32-bit binaries"
                copy_directory_files \
                        "$MESA_LIBDIR" \
                        "$TEMP_PKG_DIR/tools/${MESA_LIBDIR#objs/}" \
                        $MESA_BINARIES
            fi
        fi
        MESA_LIBDIR=objs/lib64/$EMUGL_BACKEND
        if [ -d "$MESA_LIBDIR" ]; then
            MESA_BINARIES=$(list_files_under "$MESA_LIBDIR")
            if [ "$MESA_BINARIES" ] ;then
                dump "[$PKG_NAME] Copying $EMUGL_BACKEND 64-bit binaries"
                copy_directory_files \
                        "$MESA_LIBDIR" \
                        "$TEMP_PKG_DIR/tools/${MESA_LIBDIR#objs/}" \
                        $MESA_BINARIES
            fi
        fi
    done

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
    package_archive_files "$PKG_FILE" "$TEMP_DIR"/$SYSTEM \
            $PKG_PREFIX-$PKG_REVISION
}

# Rebuild Darwin binaries remotely through SSH
# $1: Host name.
# $2: Source package file.
# $3: Darwin prebuilts directory.
build_darwin_binaries_on () {
    local HOST PKG_FILE PKG_FILE_BASENAME DST_DIR TARFLAGS
    local AOSP_DIR AOSP_PREBUILTS_DIR DARWIN_FLAGS SYSTEM
    HOST=$1
    PKG_FILE=$2
    AOSP_PREBUILTS_DIR=$3
    AOSP_DIR=$AOSP_PREBUILTS_DIR/..

    # The package file is ....../something-darwin.tar.bz2
    # And should unpack to a single directory named 'something/'
    # so extract the prefix from the package name.
    PKG_FILE_BASENAME=$(basename "$PKG_FILE")
    PKG_FILE_PREFIX=${PKG_FILE_BASENAME%%-sources.tar.bz2}
    if [ "$PKG_FILE_PREFIX" = "$PKG_FILE_BASENAME" ]; then
        # Sanity check.
        panic "Can't get package prefix from $PKG_FILE_BASENAME"
    fi

    # Where to do the work on the remote host.
    builder_prepare_remote_darwin_build \
            /tmp/$USER-android-emulator/$PKG_FILE_PREFIX

    run tar xf "$PKG_FILE" -C "$DARWIN_PKG_DIR"/..

    if [ "$AOSP_PREBUILTS_DIR" ]; then
        var_append DARWIN_BUILD_FLAGS "--aosp-prebuilts-dir=$DARWIN_REMOTE_DIR/aosp/prebuilts"
    else
        var_append DARWIN_BUILD_FLAGS "--no-aosp-prebuilts"
    fi

    cat > $DARWIN_PKG_DIR/build.sh <<EOF
#!/bin/bash -l
cd $DARWIN_REMOTE_DIR/qemu &&
./android-rebuild.sh $DARWIN_BUILD_FLAGS
EOF
    builder_run_remote_darwin_build ||
        panic "Can't rebuild binaries on Darwin, use --verbose to see why!"

    dump "Retrieving Darwin binaries from: $HOST"
    # `pwd` == external/qemu
    rm -rf objs/*
    run rsync -haz --delete --exclude=intermediates --exclude=libs $HOST:$DARWIN_REMOTE_DIR/qemu/objs .
    # TODO(digit): Retrieve PC BIOS files.
    dump "Deleting files off darwin system"
    run ssh $HOST rm -rf $DARWIN_REMOTE_DIR

    if [ ! -d "obj/qemu" ]; then
        QEMU_PREBUILTS_DIR=$PREBUILTS_DIR/qemu-android
        QEMU_BINARIES=$(list_files_under "$QEMU_PREBUILTS_DIR" "darwin-*/qemu-system-*")
        if [ "$QEMU_BINARIES" ]; then
            for QEMU_BINARY in $QEMU_BINARIES; do
                dump "[$PKG_NAME] Copying $QEMU_BINARY"
                copy_file_into "$QEMU_PREBUILTS_DIR"/$QEMU_BINARY \
                               objs/qemu/$(dirname $QEMU_BINARY)
            done
        fi
    fi

    create_binaries_package darwin
}

if [ "$DARWIN_SSH" ]; then
    build_darwin_binaries_on \
            "$DARWIN_SSH" \
            "$SOURCES_PKG_FILE" \
            "$AOSP_PREBUILTS_DIR" ||
        panic "Cannot rebuild Darwin binaries remotely"
fi

###
###  PERFORM LOCAL BUILDS
###

if [ "$AOSP_PREBUILTS_DIR" ]; then
    var_append REBUILD_FLAGS "--aosp-prebuilts-dir=$AOSP_PREBUILTS_DIR"
else
    var_append REBUILD_FLAGS "--no-aosp-prebuilts-dir"
fi

for SYSTEM in $(convert_host_list_to_os_list $LOCAL_HOST_SYSTEMS); do
    PKG_NAME="$PKG_REVISION-$SYSTEM"
    dump "[$PKG_NAME] Rebuilding binaries from sources."
    run cd $QEMU_DIR

    case $SYSTEM in
        windows)
            run ./android-rebuild.sh --mingw $REBUILD_FLAGS ||
                    panic "Use ./android-rebuild.sh --mingw to see why."
            ;;

        *)
            run ./android-rebuild.sh $REBUILD_FLAGS ||
                    panic "Use ./android-rebuild.sh to see why."
    esac

    create_binaries_package "$SYSTEM"
done

###
###  COPY PREBUILTS TO AOSP/prebuilts/android-emulator IF NEEDED
###

if [ "$OPT_COPY_PREBUILTS" ]; then
    for SYSTEM in linux darwin windows; do
        SRC_DIR="$TEMP_DIR"/$SYSTEM/$PKG_PREFIX-$PKG_REVISION
        if [ $SYSTEM = "windows" ]; then
            SYSTEM_ARCH=$SYSTEM
            BITNESS=
        else
            SYSTEM_ARCH=$SYSTEM-x86_64
            BITNESS=64
        fi
        DST_DIR=$TARGET_PREBUILTS_DIR/$SYSTEM_ARCH
        dump "[$SYSTEM_ARCH] Copying emulator binaries into $DST_DIR"
        run mkdir -p "$DST_DIR" || panic "Could not create directory: $DST_DIR"
        DLLEXT=$(dll_ext_for $SYSTEM)
        EXEEXT=$(exe_ext_for $SYSTEM)
        FILES="emulator$EXEEXT"
        for ARCH in arm x86 mips; do
            var_append FILES "emulator$BITNESS-$ARCH$EXEEXT"
        done
        var_append FILES $(list_files_under "$SRC_DIR/tools" \
                "emulator-ranchu-*$EXEEXT" \
                "emulator64-ranchu-*$EXEEXT")

        if [ -d "$SRC_DIR/tools/lib$BITNESS" ]; then
            for LIB in $EMUGL_LIBRARIES; do
                var_append FILES "lib$BITNESS/lib$BITNESS$LIB$DLLEXT"
            done
        fi

        if [ -d "$SRC_DIR/tools/qemu" ]; then
            var_append FILES $(list_files_under "$SRC_DIR/tools" "qemu/*")
        fi

        # temporarily include linux 32 bit binaries
        if [ $SYSTEM = "linux" ]; then
            BITNESS=
            for ARCH in arm x86 mips; do
                FILES="$FILES emulator-$ARCH$EXEEXT"
            done
            for LIB in $EMUGL_LIBRARIES; do
                var_append FILES "lib/lib$LIB$DLLEXT"
            done
        fi

        # Copy Emugl backend files too.
        for EMUGL_BACKEND in $EMUGL_BACKEND_DIRS; do
            EMUGL_LIBDIR=lib/$EMUGL_BACKEND
            for LIB in $(list_files_under "$SRC_DIR/tools/$EMUGL_LIBDIR"); do
                var_append FILES "$EMUGL_LIBDIR/$LIB"
            done
            EMUGL_LIBDIR=lib64/$EMUGL_BACKEND
            for LIB in $(list_files_under "$SRC_DIR/tools/$EMUGL_LIBDIR"); do
                var_append FILES "$EMUGL_LIBDIR/$LIB"
            done
        done

        copy_directory_files "$SRC_DIR/tools" "$DST_DIR" "$FILES" ||
                panic "Could not copy binaries to $DST_DIR"
    done
    README_FILE=$TARGET_PREBUILTS_DIR/README
    cat > $README_FILE <<EOF
This directory contains prebuilt emulator binaries that were generated by
running the following command on a 64-bit Linux machine:

  external/qemu/android/scripts/package-release.sh \\
      --darwin-ssh=<host> \\
      --copy-prebuilts=<path>

Where <host> is the host name of a Darwin machine, and <path> is the root
path of this AOSP repo workspace.

Below is the list of specific commits for each input directory used:

EOF
    for AOSP_SUBDIR in $AOSP_SOURCE_SUBDIRS; do
        printf "%-20s %s\n" "$AOSP_SUBDIR" "$(_get_aosp_subdir_commit_description $AOSP_SUBDIR)" >> $README_FILE
    done

    cat >> $README_FILE <<EOF

Summary of changes:

EOF
    for AOSP_SUBDIR in $AOSP_SOURCE_SUBDIRS; do
        VARNAME=$(_get_aosp_subdir_varname $AOSP_SUBDIR)
        CUR_SHA1=$(var_value $VARNAME)
        PREV_SHA1=$(var_value PREV_$VARNAME)
        if [ "$CUR_SHA1" != "$PREV_SHA1" ]; then
            GIT_LOG_COMMAND="cd $AOSP_SUBDIR && git log --oneline --no-merges $PREV_SHA1..$CUR_SHA1 ."
            printf "    $ %s\n" "$GIT_LOG_COMMAND" >> $README_FILE
            (cd $QEMU_DIR/../.. && eval $GIT_LOG_COMMAND) | while read LINE; do
                printf "        %s\n" "$LINE" >> $README_FILE
            done
            printf "\n" >> $README_FILE
        else
            cat >> $README_FILE <<EOF
    # No changes to $AOSP_SUBDIR
EOF
        fi
    done
fi

###
###  DONE
###

dump "Done. See $PKG_DIR"
ls -lh "$PKG_DIR"/$PKG_PREFIX-$PKG_REVISION*

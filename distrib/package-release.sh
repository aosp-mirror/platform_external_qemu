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

# Same as 'run', but slightly more quiet:
#  0 or 1 -> Don't display anything
#  2      -> Diplay the command, its output and error.
run2 () {
    case $VERBOSE in
        0|1)
            "$@" >/dev/null 2>&1
            ;;
        2)
            echo "COMMAND: $@"
            "$@" >/dev/null 2>&1
            ;;
        *)
            echo "COMMAND: $@"
            "$@"
            ;;
    esac
}

# Return the value of a given named variable.
# $1: Variable name.
# Out: Variable value as single string
var_value () {
    eval printf %s \"\$$1\"
}

# Assign a named variable to a specific value.
# $1: Variable name
# $2+: Variable value
var_assign () {
    local __var_assign_name=$1
    local __var_assign_value
    shift
    # Escape any single quote in the value.
    __var_assign_value=$(printf "%s" "$*" | sed -e "s/'/'\"'\"'/g")
    # Assign the value to the variable.
    eval $__var_assign_name=\'"$__var_assign_value"\'
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

# Rebuild Darwin binaries remotely through SSH
# $1: Host name.
# $2: Source package file.
build_darwin_binaries_on () {
  local HOST PKG_FILE PKG_FILE_BASENAME DST_DIR TARFLAGS
  HOST=$1
  PKG_FILE=$2

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
  DST_DIR=/tmp/android-emulator-build

  if [ "$VERBOSE" -ge 3 ]; then
    TARFLAGS="v"
  fi
  dump "Copying sources to Darwin host: $HOST"
  run ssh $HOST "mkdir -p $DST_DIR && rm -rf $DST_DIR/$PKG_FILE_BASENAME"
  cat "$PKG_FILE" | ssh $HOST "cd $DST_DIR && tar x${TARGFLAGS}f -"

  dump "Rebuilding Darwin binaries remotely."
  run ssh $HOST "bash -l -c \"cd $DST_DIR/$PKG_FILE_PREFIX/qemu && ./android-rebuild.sh $REBUILD_FLAGS\"" ||
        panic "Can't rebuild binaries on Darwin, use --verbose to see why!"

  dump "Retrieving Darwin binaries from: $HOST"
  # `pwd` == external/qemu
  rm -rf objs/*
  run rsync -haz --delete --exclude=intermediates --exclude=libs $HOST:$DST_DIR/$PKG_FILE_PREFIX/qemu/objs .
  # TODO(digit): Retrieve PC BIOS files.
  dump "Deleting files off darwin system"
  run ssh $HOST rm -rf $DST_DIR/$PKG_FILE_PREFIX
}

# Extract the git commit SHA1 of a given directory, and put its value
# in a destination variable. If the target directory is not the root
# of a git checkout, abort.
# $1: Destination variable name.
# $2: Git directory.
# Example:   extract_commit_description GTEST_DESC "$GTEST_DIR"
extract_git_commit_description () {
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

# Defaults.
DEFAULT_REVISION=$(date +%Y%m%d)
DEFAULT_PKG_PREFIX=android-emulator
DEFAULT_PKG_DIR=/tmp
DEFAULT_DARWIN_SSH=$ANDROID_EMULATOR_DARWIN_SSH

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
OPT_COPY_PREBUILTS=
OPT_DARWIN_SSH=
OPT_DEBUG=
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
        --copy-prebuilts=*)
            OPT_COPY_PREBUILTS=${OPT##--copy-prebuilts=}
            ;;
        --darwin-ssh=*)
            OPT_DARWIN_SSH=${OPT##--darwin-ssh=}
            ;;
        --debug)
            OPT_DEBUG=true
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

Use --darwin-ssh=<host> to build perform a remote build of the Darwin
binaries on a remote host through ssh. Note that this forces --sources
as well. You can also define ANDROID_EMULATOR_DARWIN_SSH in your
environment to setup a default value for this option.

Use --copy-prebuilts=<path> to specify the path of an AOSP workspace/checkout,
and to copy 64-bit prebuilt binaries to <path>/prebuilts/android-emulator/
for both Linux and Darwin platforms. This option requires the use of
--darwin-ssh=<host> or ANDROID_EMULATOR_DARWIN_SSH to build the Darwin
binaries.

Valid options (defaults are inside brackets):
    --help | -?           Print this message.
    --package-dir=<path>  Change package output directory [$DEFAULT_PKG_DIR].
    --revision=<name>     Change revision [$DEFAULT_REVISION].
    --sources             Also create sources package.
    --system=<list>       Specify host system list [$DEFAULT_SYSTEMS].
    --copy-prebuilts=<path>  Copy 64-bit Linux and Darwin binaries to
                             <path>/prebuilts/android-emulator/

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

if [ -z "$OPT_DARWIN_SSH" ]; then
  DARWIN_SSH=$DEFAULT_DARWIN_SSH
  if [ "$DARWIN_SSH" ]; then
    log "Auto-config: --darwin-ssh=$DARWIN_SSH  (from environment)."
  fi
else
  DARWIN_SSH=$OPT_DARWIN_SSH
fi

if [ "$DARWIN_SSH" ]; then
    if [ -z "$OPT_SOURCES" ]; then
        OPT_SOURCES=true
        log "Auto-config: --sources  (remote Darwin build)."
    fi
    SYSTEMS="$SYSTEMS darwin"
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
elif [ -f "$PROGDIR/../../../build/envsetup.sh" ]; then
    TARGET_AOSP=$(cd $PROGDIR/../../.. && pwd -P)
    log "Found AOSP checkout directory: $TARGET_AOSP"
    TARGET_PREBUILTS_DIR=$TARGET_AOSP/prebuilts/android-emulator
    if [ ! -d "$TARGET_PREBUILTS_DIR" ]; then
        TARGET_PREBUILTS_DIR=
    fi
else
    TARGET_AOSP=$(cd $PROGDIR/../../.. && pwd -P)
fi

case $VERBOSE in
  0|1)
    REBUILD_FLAGS=""
    ;;
  2)
    REBUILD_FLAGS="--verbose"
    ;;
  *)
    REBUILD_FLAGS="--verbose --verbose"
    ;;
esac

if [ "$OPT_DEBUG" ]; then
    REBUILD_FLAGS="$REBUILD_FLAGS --debug"
fi

# Remove duplicates.
SYSTEMS=$(echo "$SYSTEMS" | tr ' ' '\n' | sort -u | tr '\n' ' ')
log "Building for the following systems: $SYSTEMS"

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

get_aosp_subdir_varname () {
    local __aosp_subdir="$1"
    echo "AOSP_COMMIT_$(printf "%s" "$__aosp_subdir" | tr '/' '_')"
}

get_aosp_subdir_commit () {
    var_value $(get_aosp_subdir_varname $1)
}

get_aosp_subdir_commit_description () {
    var_value $(get_aosp_subdir_varname $1)__TEXT
}

# $1: Variable name
# $2: Path to README file
# $3: AOSP source sub-directory (e.g. external/qemu)
extract_previous_git_commit_from_readme () {
    local SHA1
    SHA1=$(awk '$1 == "'$3'" { print $2; }' "$2")
    var_assign $1 "$SHA1"
}

# $1: AOSP sub-directory.
extract_subdir_git_history () {
    local VARNAME="$(get_aosp_subdir_varname $1)"
    local SUBDIR="$TARGET_AOSP/$1"
    if [ ! -d "$SUBDIR" ]; then
        panic "Missing required source directory: $SUBDIR"
    fi
    log "Found source directory: $SUBDIR"
    extract_git_commit_description $VARNAME "$SUBDIR"
    log "Found current $1 commit: $(var_value $VARNAME)"
    # If there is an old prebuilts directory somewhere, read
    # the old README file to extract the previous commits there.
    if [ "$TARGET_PREBUILTS_DIR" ]; then
        README_FILE=$TARGET_PREBUILTS_DIR/README
        if [ -f "$README_FILE" ]; then
            extract_previous_git_commit_from_readme \
                PREV_$VARNAME "$README_FILE" "$1"
            log "Found previous $1 commit: $(var_value PREV_$VARNAME)"
        fi
    fi
}

# The list of AOSP directories that contain relevant sources for this
# script.
AOSP_SOURCE_SUBDIRS="external/qemu sdk/emulator/opengl external/gtest"

for AOSP_SUBDIR in $AOSP_SOURCE_SUBDIRS; do
    extract_subdir_git_history $AOSP_SUBDIR
done

SOURCES_PKG_FILE=
if [ "$OPT_SOURCES" ]; then
    BUILD_DIR=$TEMP_BUILD_DIR/sources/$PKG_PREFIX-$PKG_REVISION
    PKG_NAME="$PKG_REVISION-sources"
    for AOSP_SUBDIR in $AOSP_SOURCE_SUBDIRS; do
        dump "[$PKG_NAME] Copying $AOSP_SUBDIR source files."
        copy_directory_git_files "$TARGET_AOSP/$AOSP_SUBDIR" "$BUILD_DIR"/$(basename $AOSP_SUBDIR)
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
    (run cd "$BUILD_DIR"/.. && run tar cjf "$PKG_FILE" $PKG_PREFIX-$PKG_REVISION)
fi

for SYSTEM in $SYSTEMS; do
    PKG_NAME="$PKG_REVISION-$SYSTEM"
    dump "[$PKG_NAME] Rebuilding binaries from sources."
    run cd $QEMU_DIR
    case $SYSTEM in
        $HOST_SYSTEM)
            run ./android-rebuild.sh $REBUILD_FLAGS || panic "Use ./android-rebuild.sh to see why."
            ;;
        darwin)
            if [ -z "$DARWIN_SSH" ]; then
                # You can only rebuild Darwin binaries on non-Darwin systems
                # by using --darwin-ssh=<host>.
                panic "You can only rebuild Darwin binaries with --darwin-ssh"
            fi
            if [ -z "$SOURCES_PKG_FILE" ]; then
                panic "You must use --sources to build Darwin binaries through ssh"
            fi
            build_darwin_binaries_on "$DARWIN_SSH" "$SOURCES_PKG_FILE"
            ;;
        windows)
            if [ "$HOST_SYSTEM" != "linux" ]; then
                panic "Windows binaries can only be rebuilt on Linux!"
            fi
            run ./android-rebuild.sh --mingw $REBUILD_FLAGS || panic "Use ./android-rebuild.sh --mingw to see why."
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
        run2 cp -rp objs/lib/* "$TEMP_PKG_DIR"/tools/lib/
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
    (run cd "$TEMP_BUILD_DIR"/$SYSTEM && run tar cjf $PKG_FILE $PKG_PREFIX-$PKG_REVISION)
done

if [ "$OPT_COPY_PREBUILTS" ]; then
    for SYSTEM in linux darwin windows; do
        SRC_DIR="$TEMP_BUILD_DIR"/$SYSTEM/$PKG_PREFIX-$PKG_REVISION
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
        EXEEXT=
        case $SYSTEM in
            linux) DLLEXT=.so;;
            darwin) DLLEXT=.dylib;;
            windows)
              DLLEXT=.dll
              EXEEXT=.exe
              ;;
            *) panic "Unsupported prebuilt system: $SYSTEM";;
        esac
        FILES="emulator$EXEEXT"
        for ARCH in arm x86 mips; do
            FILES="$FILES emulator$BITNESS-$ARCH$EXEEXT"
        done
        for ARCH in arm64; do
            if [ -f "$SRC_DIR/tools/emulator64-$ARCH$EXEEXT" ]; then
                FILES="$FILES emulator64-$ARCH$EXEEXT"
            fi
        done

        for LIB in OpenglRender EGL_translator GLES_CM_translator GLES_V2_translator; do
            FILES="$FILES lib/lib$BITNESS$LIB$DLLEXT"
        done

        # temparily include linux 32 bit binaries
        if [ $SYSTEM = "linux" ]; then
            BITNESS=
            for ARCH in arm x86 mips; do
                FILES="$FILES emulator$BITNESS-$ARCH$EXEEXT"
            done
            for LIB in OpenglRender EGL_translator GLES_CM_translator GLES_V2_translator; do
                FILES="$FILES lib/lib$BITNESS$LIB$DLLEXT"
            done
        fi


        (cd "$SRC_DIR/tools" && tar cf - $FILES) | (cd $DST_DIR && tar xf -) ||
                panic "Could not copy binaries to $DST_DIR"
    done
    README_FILE=$TARGET_PREBUILTS_DIR/README
    cat > $README_FILE <<EOF
This directory contains prebuilt emulator binaries that were generated by
running the following command on a 64-bit Linux machine:

  external/qemu/distrib/package-release.sh \\
      --darwin-ssh=<host> \\
      --copy-prebuilts=<path>

Where <host> is the host name of a Darwin machine, and <path> is the root
path of this AOSP repo workspace.

Below is the list of specific commits for each input directory used:

EOF
    for AOSP_SUBDIR in $AOSP_SOURCE_SUBDIRS; do
        printf "%-20s %s\n" "$AOSP_SUBDIR" "$(get_aosp_subdir_commit_description $AOSP_SUBDIR)" >> $README_FILE
    done

    cat >> $README_FILE <<EOF

Summary of changes:

EOF
    for AOSP_SUBDIR in $AOSP_SOURCE_SUBDIRS; do
        VARNAME=$(get_aosp_subdir_varname $AOSP_SUBDIR)
        CUR_SHA1=$(var_value $VARNAME)
        PREV_SHA1=$(var_value PREV_$VARNAME)
        if [ "$CUR_SHA1" != "$PREV_SHA1" ]; then
            GIT_LOG_COMMAND="cd $AOSP_SUBDIR && git log --oneline --no-merges $PREV_SHA1..$CUR_SHA1 ."
            printf "    $ %s\n" "$GIT_LOG_COMMAND" >> $README_FILE
            (cd $TARGET_AOSP && eval $GIT_LOG_COMMAND) | while read LINE; do
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

dump "Done. See $PKG_DIR"
ls -lh "$PKG_DIR"/$PKG_PREFIX-$PKG_REVISION*

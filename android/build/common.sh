# Copyright (C) 2008 The Android Open Source Project
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
#
# This file is included by other shell scripts; do not execute it directly.
# It contains common definitions.
#
PROGNAME=`basename $0`
PROGDIR=`dirname $0`

## Logging support
##
VERBOSE=yes
VERBOSE2=no

panic ()
{
    echo "ERROR: $@"
    exit 1
}

log ()
{
    if [ "$VERBOSE" = "yes" ] ; then
        echo "$1"
    fi
}

log2 ()
{
    if [ "$VERBOSE2" = "yes" ] ; then
        echo "$1"
    fi
}

## Normalize OS and CPU
##

CPU=`uname -m`
case "$CPU" in
    i?86) CPU=x86
    ;;
    amd64) CPU=x86_64
    ;;
    powerpc) panic "PowerPC builds are not supported!"
    ;;
esac

log2 "CPU=$CPU"

# at this point, the supported values for CPU are:
#   x86
#   x86_64
#
# other values may be possible but haven't been tested
#

EXE=""
OS=`uname -s`
case "$OS" in
    Darwin)
        OS=darwin-$CPU
        ;;
    Linux)
        # note that building  32-bit binaries on x86_64 is handled later
        OS=linux-$CPU
	;;
    FreeBSD)
        OS=freebsd-$CPU
        ;;
    CYGWIN*|*_NT-*)
        panic "Please build Windows binaries on Linux with --mingw option."
        ;;
esac

log2 "OS=$OS"
log2 "EXE=$EXE"

# at this point, the value of OS should be one of the following:
#   linux-x86
#   linux-x86_64
#   darwin-x86
#   darwin-x86_64
#
# other values may be possible but have not been tested

# define HOST_OS as $OS without any cpu-specific suffix
#
case $OS in
    linux-*) HOST_OS=linux
    ;;
    darwin-*) HOST_OS=darwin
    ;;
    freebsd-*) HOST_OS=freebsd
    ;;
    *) HOST_OS=$OS
esac

# define HOST_ARCH as the $CPU
HOST_ARCH=$CPU

# define HOST_TAG
# special case: windows-x86 => windows
compute_host_tag ()
{
    case $HOST_OS-$HOST_ARCH in
        windows-x86)
            HOST_TAG=windows
            ;;
        *)
            HOST_TAG=$HOST_OS-$HOST_ARCH
            ;;
    esac
}
compute_host_tag

#### Toolchain support
####

WINDRES=

# Various probes are going to need to run a small C program
TMPC=/tmp/android-$$-test.c
TMPO=/tmp/android-$$-test.o
TMPE=/tmp/android-$$-test$EXE
TMPL=/tmp/android-$$-test.log

# cleanup temporary files
clean_temp ()
{
    rm -f $TMPC $TMPO $TMPL $TMPE
}

# cleanup temp files then exit with an error
clean_exit ()
{
    clean_temp
    exit 1
}

# this function should be called to enforce the build of 32-bit binaries on 64-bit systems
# that support it.
FORCE_32BIT=no
force_32bit_binaries ()
{
    if [ $CPU = x86_64 ] ; then
        FORCE_32BIT=yes
        case $OS in
            linux-x86_64) OS=linux-x86 ;;
            darwin-x86_64) OS=darwin-x86 ;;
            freebsd-x86_64) OS=freebsd-x86 ;;
        esac
        HOST_ARCH=x86
        CPU=x86
        compute_host_tag
        log "Check32Bits: Forcing generation of 32-bit binaries (--try-64 to disable)"
    fi
}

# Enable linux-mingw32 compilation. This allows you to build
# windows executables on a Linux machine, which is considerably
# faster than using Cygwin / MSys to do the same job.
#
enable_linux_mingw ()
{
    # Are we on Linux ?
    log "Mingw      : Checking for Linux host"
    if [ "$HOST_OS" != "linux" ] ; then
        echo "Sorry, but mingw compilation is only supported on Linux !"
        exit 1
    fi
    # Do we have our prebuilt mingw64 toolchain?
    log "Mingw      : Looking for prebuilt mingw64 toolchain."
    MINGW_DIR=$PROGDIR/../../prebuilts/gcc/linux-x86/host/x86_64-w64-mingw32-4.8
    MINGW_CC=
    if [ -d "$MINGW_DIR" ]; then
        MINGW_PREFIX=$MINGW_DIR/bin/x86_64-w64-mingw32
        find_program MINGW_CC "$MINGW_PREFIX-gcc"
    fi
    if [ -z "$MINGW_CC" ]; then
        log "Mingw      : Looking for mingw64 toolchain."
        MINGW_PREFIX=x86_64-w64-mingw32
        find_program MINGW_CC $MINGW_PREFIX-gcc
    fi
    if [ -z "$MINGW_CC" ]; then
        echo "ERROR: It looks like no Mingw64 toolchain is available!"
        echo "Please install x86_64-w64-mingw32 package !"
        exit 1
    fi
    log2 "Mingw      : Found $MINGW_CC"
    CC=$MINGW_CC
    LD=$MINGW_CC
    WINDRES=$MINGW_PREFIX-windres
    AR=$MINGW_PREFIX-ar
}

# this function will setup the compiler and linker and check that they work as advertized
# note that you should call 'force_32bit_binaries' before this one if you want it to work
# as advertized.
#
setup_toolchain ()
{
    if [ -z "$CC" ] ; then
        CC=gcc
    fi

    # check that we can compile a trivial C program with this compiler
    cat > $TMPC <<EOF
int main(void) {}
EOF

    if [ $FORCE_32BIT = yes ] ; then
        CFLAGS="$CFLAGS -m32"
        LDFLAGS="$LDFLAGS -m32"
        compile
        if [ $? != 0 ] ; then
            # sometimes, we need to also tell the assembler to generate 32-bit binaries
            # this is highly dependent on your GCC installation (and no, we can't set
            # this flag all the time)
            CFLAGS="$CFLAGS -Wa,--32"
        fi
    fi

    compile
    if [ $? != 0 ] ; then
        echo "your C compiler doesn't seem to work: $CC"
        cat $TMPL
        clean_exit
    fi
    log "CC         : compiler check ok ($CC)"
    CC_VER=`$CC --version`
    log "CC_VER     : $CC_VER"

    # check that we can link the trivial program into an executable
    if [ -z "$LD" ] ; then
        LD=$CC
    fi
    link
    if [ $? != 0 ] ; then
        echo "your linker doesn't seem to work:"
        cat $TMPL
        clean_exit
    fi
    log "LD         : linker check ok ($LD)"

    if [ -z "$AR" ]; then
        AR=ar
    fi
    log "AR         : archiver ($AR)"
    clean_temp
}

# try to compile the current source file in $TMPC into an object
# stores the error log into $TMPL
#
compile ()
{
    log2 "Object     : $CC -o $TMPO -c $CFLAGS $TMPC"
    $CC -o $TMPO -c $CFLAGS $TMPC 2> $TMPL
}

# try to link the recently built file into an executable. error log in $TMPL
#
link()
{
    log2 "Link      : $LD -o $TMPE $TMPO $LDFLAGS"
    $LD -o $TMPE $TMPO $LDFLAGS 2> $TMPL
}

# perform a simple compile / link / run of the source file in $TMPC
compile_exec_run()
{
    log2 "RunExec    : $CC -o $TMPE $CFLAGS $TMPC"
    compile
    if [ $? != 0 ] ; then
        echo "Failure to compile test program"
        cat $TMPC
        cat $TMPL
        clean_exit
    fi
    link
    if [ $? != 0 ] ; then
        echo "Failure to link test program"
        cat $TMPC
        echo "------"
        cat $TMPL
        clean_exit
    fi
    $TMPE
}

## Feature test support
##

# check that a given C header file exists on the host system
# $1: variable name which will be set to "yes" or "no" depending on result
# $2: header name
#
# you can define EXTRA_CFLAGS for extra C compiler flags
# for convenience, this variable will be unset by the function.
#
feature_check_header ()
{
    local result_ch OLD_CFLAGS
    log2 "HeaderCheck: $2"
    echo "#include $2" > $TMPC
    cat >> $TMPC <<EOF
        int main(void) { return 0; }
EOF
    OLD_CFLAGS=$CFLAGS
    CFLAGS="$CFLAGS $EXTRA_CFLAGS"
    compile
    if [ $? != 0 ]; then
        result_ch=no
    else
        result_ch=yes
    fi
    log "HeaderCheck: $2 [$result_ch]"
    EXTRA_CFLAGS=
    CFLAGS=$OLD_CFLAGS
    eval $1=$result_ch
    clean_temp
}

# run the test program that is in $TMPC and set its exit status
# in the $1 variable.
# you can define EXTRA_CFLAGS and EXTRA_LDFLAGS
#
feature_run_exec ()
{
    local run_exec_result
    local OLD_CFLAGS="$CFLAGS"
    local OLD_LDFLAGS="$LDFLAGS"
    CFLAGS="$CFLAGS $EXTRA_CFLAGS"
    LDFLAGS="$LDFLAGS $EXTRA_LDFLAGS"
    compile_exec_run
    run_exec_result=$?
    CFLAGS="$OLD_CFLAGS"
    LDFLAGS="$OLD_LDFLAGS"
    eval $1=$run_exec_result
    log "Host       : $1=$run_exec_result"
    clean_temp
}

## Build configuration file support
## you must define $config_mk before calling this function
##
## $1: Optional output directory.
create_config_mk ()
{
    # create the directory if needed
    local  config_dir
    local out_dir=${1:-objs}
    config_mk=${config_mk:-$out_dir/config.make}
    config_dir=`dirname $config_mk`
    mkdir -p $config_dir 2> $TMPL
    if [ $? != 0 ] ; then
        echo "Can't create directory for build config file: $config_dir"
        cat $TMPL
        clean_exit
    fi

    # re-create the start of the configuration file
    log "Generate   : $config_mk"

    echo "# This file was autogenerated by $PROGNAME. Do not edit !" > $config_mk
    echo "OS          := $OS" >> $config_mk
    echo "HOST_OS     := $HOST_OS" >> $config_mk
    echo "HOST_ARCH   := $HOST_ARCH" >> $config_mk
    echo "CC          := $CC" >> $config_mk
    echo "LD          := $LD" >> $config_mk
    echo "AR          := $AR" >> $config_mk
    echo "CFLAGS      := $CFLAGS" >> $config_mk
    echo "LDFLAGS     := $LDFLAGS" >> $config_mk
    echo "HOST_CC     := $CC" >> $config_mk
    echo "HOST_LD     := $LD" >> $config_mk
    echo "HOST_AR     := $AR" >> $config_mk
    echo "HOST_WINDRES:= $WINDRES" >> $config_mk
    echo "OBJS_DIR    := $out_dir" >> $config_mk
}

# Find pattern $1 in string $2
# This is to be used in if statements as in:
#
#    if pattern_match <pattern> <string>; then
#       ...
#    fi
#
pattern_match ()
{
    echo "$2" | grep -q -E -e "$1"
}

# Find if a given shell program is available.
# We need to take care of the fact that the 'which <foo>' command
# may return either an empty string (Linux) or something like
# "no <foo> in ..." (Darwin). Also, we need to redirect stderr
# to /dev/null for Cygwin
#
# $1: variable name
# $2: program name
#
# Result: set $1 to the full path of the corresponding command
#         or to the empty/undefined string if not available
#
find_program ()
{
    local PROG
    PROG=`which $2 2>/dev/null || true`
    if [ -n "$PROG" ] ; then
        if pattern_match '^no ' "$PROG"; then
            PROG=
        fi
    fi
    eval $1="$PROG"
}

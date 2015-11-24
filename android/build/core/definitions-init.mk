# Copyright (C) 2009-2010 The Android Open Source Project
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

# Build system initialization.
#
# Input variables:
#    BUILD_SYSTEM_ROOT
#       Directory containing the root build system directories, i.e.
#       'gmsl', 'core', 'awk'.
#
#    BUILD_SYSTEM_NAME
#       Default prefix used in build system logs. Default value is 'Build'.
#
#    BUILD_LOG
#       Set to '1' to enable internal build system logs.
#
#    BUILD_SYSTEM_PREBUILT_DIR
#       If defined, must point to a directory that will contain host
#       tool binaries under $(BUILD_SYSTEM_PREBUILT_DIR)/<host>/bin/
#       where <host> can be the 32-bit or 64-bit host tag, depending
#       on the value of BUILD_FORCE_HOST_32BIT (see below).
#
#    BUILD_FORCE_HOST_32BIT
#       Set to '1' to force the use of 32-bit host prebuilt tool binaries,
#       when BUILD_SYSTEM_PREBUILT_DIR is defined (ignored otherwise)
#
#    BUILD_HOST_OS
#       Host operating system name. If undefined, will be auto-detected.
#       Examples: linux, darwin, windows
#
#    BUILD_HOST_ARCH
#       Host operating system architecture. If undefined will be auto-detected.
#       Examples: x86, x86_64
#
#    BUILD_HOST_AWK
#       Path to the host 'awk' executable. Auto-detected if undefined.
#
#    BUILD_HOST_CMP
#       Path to the host 'cmp' executable. Auto-detected if undefined.
#
#    BUILD_HOST_ECHO
#       Path to the host 'echo' executable. Auto-detected if undefined.
#
#    BUILD_HOST_PYTHON
#       Path to the host 'python' executable. Auto-detected if undefined.

# Disable GNU Make implicit rules

# this turns off the suffix rules built into make
.SUFFIXES:

# this turns off the RCS / SCCS implicit rules of GNU Make
% : RCS/%,v
% : RCS/%
% : %,v
% : s.%
% : SCCS/s.%

# If a rule fails, delete $@.
.DELETE_ON_ERROR:

# ====================================================================
#
# Define a few useful variables and functions.
# More stuff will follow in definitions.mk.
#
# ====================================================================

# Used to output warnings and error from the library, it's possible to
# disable any warnings or errors by overriding these definitions
# manually or by setting BUILD_NO_WARNINGS.

BUILD_SYSTEM_NAME := $(strip $(BUILD_SYSTEM_NAME))
ifndef BUILD_SYSTEM_NAME
    BUILD_SYSTEM_NAME := Build
endif

_build_name    := $(BUILD_SYSTEM_NAME)
-build-info     = $(info $(_build_name): $1 $2 $3 $4 $5)
-build-warning  = $(warning $(_build_name): $1 $2 $3 $4 $5)
-build-error    = $(error $(_build_name): $1 $2 $3 $4 $5)

ifdef BUILD_NO_WARNINGS
-build-warning :=
endif

# -----------------------------------------------------------------------------
# Function : -build-log
# Arguments: 1: text to print when BUILD_LOG is defined to 1
# Returns  : None
# Usage    : $(call -build-log,<some text>)
# -----------------------------------------------------------------------------
ifeq ($(BUILD_LOG),1)
_build_log := 1
-build-log = $(info $(_build_name): $1)
else
_build_log := 0
-build-log :=
endif

# Check that we have at least GNU Make 3.81
# We do this by detecting whether 'lastword' is supported
#
MAKE_TEST := $(lastword a b c d e f)
ifneq ($(MAKE_TEST),f)
    $(call -build-error,GNU Make version $(MAKE_VERSION) is too low (should be >= 3.81))
endif
$(call -build-log, GNU Make version $(MAKE_VERSION) detected)


BUILD_SYSTEM_PREBUILT_DIR := $(strip $(BUILD_SYSTEM_PREBUILT_DIR))
ifdef BUILD_SYSTEM_PREBUILT_DIR
    ifeq (,$(strip $(wildcard $(BUILD_SYSTEM_PREBUILT_DIR))))
        $(error Missing prebuilts directory: $(BUILD_SYSTEM_PREBUILT_DIR))
    endif
endif

# ====================================================================
#
# Host system auto-detection.
#
# ====================================================================

#
# Determine host system and architecture from the environment
#
BUILD_HOST_OS := $(strip $(BUILD_HOST_OS))
ifndef BUILD_HOST_OS
    # On all modern variants of Windows (including Cygwin and Wine)
    # the OS environment variable is defined to 'Windows_NT'
    #
    # The value of PROCESSOR_ARCHITECTURE will be x86 or AMD64
    #
    ifeq ($(OS),Windows_NT)
        BUILD_HOST_OS := windows
    else
        # For other systems, use the `uname` output
        UNAME := $(shell uname -s)
        ifneq (,$(findstring Linux,$(UNAME)))
            BUILD_HOST_OS := linux
        endif
        ifneq (,$(findstring Darwin,$(UNAME)))
            BUILD_HOST_OS := darwin
        endif
        # We should not be there, but just in case !
        ifneq (,$(findstring CYGWIN,$(UNAME)))
            BUILD_HOST_OS := windows
        endif
        ifeq ($(BUILD_HOST_OS),)
            $(call -build-info,Unable to determine BUILD_HOST_OS from uname -s: $(UNAME))
            $(call -build-info,Please define BUILD_HOST_OS in your environment.)
            $(call -build-error,Aborting.)
        endif
    endif
    $(call -build-log,Host OS was auto-detected: $(BUILD_HOST_OS))
else
    $(call -build-log,Host OS from environment: $(BUILD_HOST_OS))
endif

# For all systems, we will have BUILD_HOST_OS_BASE defined as
# $(BUILD_HOST_OS), except on Cygwin where we will have:
#
#  BUILD_HOST_OS      == cygwin
#  BUILD_HOST_OS_BASE == windows
#
# Trying to detect that we're running from Cygwin is tricky
# because we can't use $(OSTYPE): It's a Bash shell variable
# that is not exported to sub-processes, and isn't defined by
# other shells (for those with really weird setups).
#
# Instead, we assume that a program named /bin/uname.exe
# that can be invoked and returns a valid value corresponds
# to a Cygwin installation.
#
BUILD_HOST_OS_BASE := $(BUILD_HOST_OS)

ifeq ($(BUILD_HOST_OS),windows)
    ifneq (,$(strip $(wildcard /bin/uname.exe)))
        $(call -build-log,Found /bin/uname.exe on Windows host, checking for Cygwin)
        # NOTE: The 2>NUL here is for the case where we're running inside the
        #       native Windows shell. On cygwin, this will create an empty NUL file
        #       that we're going to remove later (see below).
        UNAME := $(shell /bin/uname.exe -s 2>NUL)
        $(call -build-log,uname -s returned: $(UNAME))
        ifneq (,$(filter CYGWIN%,$(UNAME)))
            $(call -build-log,Cygwin detected: $(shell uname -a))
            BUILD_HOST_OS := cygwin
            DUMMY := $(shell rm -f NUL) # Cleaning up
        else
            ifneq (,$(filter MINGW32%,$(UNAME)))
                $(call -build-log,MSys detected: $(shell uname -a))
                BUILD_HOST_OS := cygwin
            else
                $(call -build-log,Cygwin *not* detected!)
            endif
        endif
    endif
endif

ifneq ($(BUILD_HOST_OS),$(BUILD_HOST_OS_BASE))
    $(call -build-log, Host operating system detected: $(BUILD_HOST_OS), base OS: $(BUILD_HOST_OS_BASE))
else
    $(call -build-log, Host operating system detected: $(BUILD_HOST_OS))
endif

# Always use /usr/bin/file on Darwin to avoid relying on broken Ports
# version. See http://b.android.com/53769 .
BUILD_HOST_FILE_PROGRAM := file
ifeq ($(BUILD_HOST_OS),darwin)
BUILD_HOST_FILE_PROGRAM := /usr/bin/file
endif

BUILD_HOST_ARCH := $(strip $(BUILD_HOST_ARCH))
BUILD_HOST_ARCH64 :=
ifndef BUILD_HOST_ARCH
    ifeq ($(BUILD_HOST_OS_BASE),windows)
        BUILD_HOST_ARCH := $(PROCESSOR_ARCHITECTURE)
        ifeq ($(BUILD_HOST_ARCH),AMD64)
            BUILD_HOST_ARCH := x86
        endif
        # Windows is 64-bit if either ProgramW6432 or ProgramFiles(x86) is set
        ifneq ("/",$(shell echo "%ProgramW6432%/%ProgramFiles(x86)%"))
            BUILD_HOST_ARCH64 := x86_64
        endif
    else # BUILD_HOST_OS_BASE != windows
        UNAME := $(shell uname -m)
        ifneq (,$(findstring 86,$(UNAME)))
            BUILD_HOST_ARCH := x86
            ifneq (,$(shell $(BUILD_HOST_FILE_PROGRAM) -L $(SHELL) | grep 'x86[_-]64'))
                BUILD_HOST_ARCH64 := x86_64
            endif
        endif
        # We should probably should not care at all
        ifneq (,$(findstring Power,$(UNAME)))
            BUILD_HOST_ARCH := ppc
        endif
        ifeq ($(BUILD_HOST_ARCH),)
            $(call -build-info,Unsupported host architecture: $(UNAME))
            $(call -build-error,Aborting)
        endif
    endif # BUILD_HOST_OS_BASE != windows
    $(call -build-log,Host CPU was auto-detected: $(BUILD_HOST_ARCH))
else
    $(call -build-log,Host CPU from environment: $(BUILD_HOST_ARCH))
endif

ifeq (,$(BUILD_HOST_ARCH64))
    BUILD_HOST_ARCH64 := $(BUILD_HOST_ARCH)
endif

BUILD_HOST_TAG := $(BUILD_HOST_OS_BASE)-$(BUILD_HOST_ARCH)
BUILD_HOST_TAG64 := $(BUILD_HOST_OS_BASE)-$(BUILD_HOST_ARCH64)

# The directory separator used on this host
BUILD_HOST_DIRSEP := :
ifeq ($(BUILD_HOST_OS),windows)
  BUILD_HOST_DIRSEP := ;
endif

# The host executable extension
BUILD_HOST_EXEEXT :=
ifeq ($(BUILD_HOST_OS),windows)
  BUILD_HOST_EXEEXT := .exe
endif

# If we are on Windows, we need to check that we are not running
# Cygwin 1.5, which is deprecated and won't run our toolchain
# binaries properly.
#
ifeq ($(BUILD_HOST_TAG),windows-x86)
    ifeq ($(BUILD_HOST_OS),cygwin)
        # On cygwin, 'uname -r' returns something like 1.5.23(0.225/5/3)
        # We recognize 1.5. as the prefix to look for then.
        CYGWIN_VERSION := $(shell uname -r)
        ifneq ($(filter XX1.5.%,XX$(CYGWIN_VERSION)),)
            $(call -build-info,You seem to be running Cygwin 1.5, which is not supported.)
            $(call -build-info,Please upgrade to Cygwin 1.7 or higher.)
            $(call -build-error,Aborting.)
        endif
    endif
    # special-case the host-tag
    BUILD_HOST_TAG := windows
endif

$(call -build-log,BUILD_HOST_TAG set to $(BUILD_HOST_TAG))

# Check for build-system-specific versions of our host tools
ifneq (,$(BUILD_SYSTEM_PREBUILT_DIR))
    ifeq (1,$(BUILD_FORCE_HOST_32BIT))
        BUILD_HOST_PREBUILT_DIR := $(BUILD_SYSTEM_PREBUILT_DIR)/$(BUILD_HOST_TAG)
    else
        BUILD_HOST_PREBUILT_DIR := $(BUILD_SYSTEM_PREBUILT_DIR)/$(BUILD_HOST_TAG64)
        ifeq (,$(strip $(wildcard $(BUILD_HOST_PREBUILT_DIR))))
            BUILD_HOST_PREBUILT_DIR := $(BUILD_SYSTEM_PREBUILT_DIR)/$(BUILD_HOST_TAG)
        endif
    endif
    BUILD_HOST_PREBUILT_DIR := $(strip $(wildcard $(BUILD_HOST_PREBUILT_DIR)))
else
    BUILD_HOST_PREBUILT_DIR :=
endif

BUILD_HOST_AWK := $(strip $(BUILD_HOST_AWK))
BUILD_HOST_PYTHON := $(strip $(BUILD_HOST_PYTHON))

ifdef BUILD_HOST_PREBUILT_DIR
    $(call -build-log,Host tools prebuilt directory: $(BUILD_HOST_PREBUILT_DIR))
    # The windows prebuilt binaries are for ndk-build.cmd
    # On cygwin, we must use the Cygwin version of these tools instead.
    ifneq ($(BUILD_HOST_OS),cygwin)
        ifndef BUILD_HOST_AWK
            BUILD_HOST_AWK := $(wildcard $(BUILD_HOST_PREBUILT_DIR)/awk$(BUILD_HOST_EXEEXT))
        endif
        ifndef BUILD_HOST_PYTHON
            BUILD_HOST_PYTHON := $(wildcard $(BUILD_HOST_PREBUILT_DIR)/python$(BUILD_HOST_EXEEXT))
        endif
    endif
else
    $(call -build-log,Host tools prebuilt directory not found, using system tools)
endif

BUILD_HOST_ECHO := $(strip $(BUILD_HOST_ECHO))
ifdef BUILD_HOST_PREBUILT_DIR
    ifndef BUILD_HOST_ECHO
        # Special case, on Cygwin, always use the host echo, not our prebuilt one
        # which adds \r\n at the end of lines.
        ifneq ($(BUILD_HOST_OS),cygwin)
            BUILD_HOST_ECHO := $(strip $(wildcard $(BUILD_HOST_PREBUILT_DIR)/echo$(BUILD_HOST_EXEEXT)))
        endif
    endif
endif
ifndef BUILD_HOST_ECHO
    BUILD_HOST_ECHO := echo
endif
$(call -build-log,Host 'echo' tool: $(BUILD_HOST_ECHO))

# Define BUILD_HOST_ECHO_N to perform the equivalent of 'echo -n' on all platforms.
ifeq ($(BUILD_HOST_OS),windows)
  # Our custom toolbox echo binary supports -n.
  BUILD_HOST_ECHO_N := $(BUILD_HOST_ECHO) -n
else
  # On Posix, just use bare printf.
  BUILD_HOST_ECHO_N := printf %s
endif
$(call -build-log,Host 'echo -n' tool: $(BUILD_HOST_ECHO_N))

BUILD_HOST_CMP := $(strip $(BUILD_HOST_CMP))
ifdef BUILD_HOST_PREBUILT_DIR
    ifndef BUILD_HOST_CMP
        BUILD_HOST_CMP := $(strip $(wildcard $(BUILD_HOST_PREBUILT_DIR)/cmp$(BUILD_HOST_EXEEXT)))
    endif
endif
ifndef BUILD_HOST_CMP
    BUILD_HOST_CMP := cmp
endif
$(call -build-log,Host 'cmp' tool: $(BUILD_HOST_CMP))

#
# Verify that the 'awk' tool has the features we need.
# Both Nawk and Gawk do.
#
BUILD_HOST_AWK := $(strip $(BUILD_HOST_AWK))
ifndef BUILD_HOST_AWK
    BUILD_HOST_AWK := awk
endif
$(call -build-log,Host 'awk' tool: $(BUILD_HOST_AWK))

# Location of all awk scripts we use
BUILD_AWK := $(BUILD_SYSTEM_ROOT)/awk
ifeq (,$(strip $(wildcard $(BUILD_AWK))))
    $(call -build-error,Missing awk scripts directory: $(BUILD_AWK))
endif

AWK_TEST := $(shell $(BUILD_HOST_AWK) -f $(BUILD_AWK)/check-awk.awk)
$(call -build-log,Host 'awk' test returned: $(AWK_TEST))
ifneq ($(AWK_TEST),Pass)
    $(call -build-info,Host 'awk' tool is outdated. Please define BUILD_HOST_AWK to point to Gawk or Nawk !)
    $(call -build-error,Aborting.)
endif

#
# On Cygwin/MSys, define the 'cygwin-to-host-path' function here depending on the
# environment. The rules are the following:
#
# 1/ If BUILD_USE_CYGPATH=1 and cygpath does exist in your path, cygwin-to-host-path
#    calls "cygpath -m" for each host path.  Since invoking 'cygpath -m' from GNU
#    Make for each source file is _very_ slow, this is only a backup plan in
#    case our automatic substitution function (described below) doesn't work.
#
# 2/ Generate a Make function that performs the mapping from cygwin/msys to host
#    paths through simple substitutions.  It's really a series of nested patsubst
#    calls, that loo like:
#
#     cygwin-to-host-path = $(patsubst /cygdrive/c/%,c:/%,\
#                             $(patsusbt /cygdrive/d/%,d:/%, \
#                              $1)
#    or in MSys:
#     cygwin-to-host-path = $(patsubst /c/%,c:/%,\
#                             $(patsusbt /d/%,d:/%, \
#                              $1)
#
# except that the actual definition is built from the list of mounted
# drives as reported by "mount" and deals with drive letter cases (i.e.
# '/cygdrive/c' and '/cygdrive/C')
#
ifeq ($(BUILD_HOST_OS),cygwin)
    CYGPATH := $(strip $(HOST_CYGPATH))
    ifndef CYGPATH
        $(call -build-log, Probing for 'cygpath' program)
        CYGPATH := $(strip $(shell which cygpath 2>/dev/null))
        ifndef CYGPATH
            $(call -build-log, 'cygpath' was *not* found in your path)
        else
            $(call -build-log, 'cygpath' found as: $(CYGPATH))
        endif
    endif

    ifeq ($(BUILD_USE_CYGPATH),1)
        ifndef CYGPATH
            $(call -build-info,No cygpath)
            $(call -build-error,Aborting)
        endif
        $(call -build-log, Forced usage of 'cygpath -m' through BUILD_USE_CYGPATH=1)
        cygwin-to-host-path = $(strip $(shell $(CYGPATH) -m $1))
    else
        # Call an awk script to generate a Makefile fragment used to define a function
        WINDOWS_HOST_PATH_FRAGMENT := $(shell mount | tr '\\' '/' | $(BUILD_HOST_AWK) -f $(BUILD_AWK)/gen-windows-host-path.awk)
        ifeq ($(_build_log),1)
            $(info Using cygwin substitution rules:)
            $(eval $(shell mount | tr '\\' '/' | $(BUILD_HOST_AWK) -f $(BUILD_AWK)/gen-windows-host-path.awk -vVERBOSE=1))
        endif
        $(eval cygwin-to-host-path = $(WINDOWS_HOST_PATH_FRAGMENT))
    endif
endif # BUILD_HOST_OS == cygwin

# -----------------------------------------------------------------------------
# Function : host-prebuilt-tag
# Arguments: 1: parent path of "prebuilt"
# Returns  : path $1/prebuilt/(BUILD_HOST_TAG64) exists and BUILD_FORCE_HOST_32BIT
#            isn't defined to 1, or $1/prebuilt/(BUILD_HOST_TAG)
# Usage    : $(call host-prebuilt-tag, <path>)
# Rationale: This function is used to proble available 64-bit toolchain or
#            return 32-bit one as default.  Note that BUILD_HOST_TAG64==BUILD_HOST_TAG for
#            32-bit system (or 32-bit userland in 64-bit system)
# -----------------------------------------------------------------------------
ifeq ($(BUILD_FORCE_HOST_32BIT),1)
host-prebuilt-tag = $1/prebuilt/$(BUILD_HOST_TAG)
else
host-prebuilt-tag = $(strip \
    $(if $(strip $(wildcard $1/prebuilt/$(BUILD_HOST_TAG64))),\
        $1/prebuilt/$(BUILD_HOST_TAG64),\
        $1/prebuilt/$(BUILD_HOST_TAG)\
    ))
endif

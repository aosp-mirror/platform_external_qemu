# Copyright (C) 2009 The Android Open Source Project
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
# Functions related to file and directory creation
#

# -----------------------------------------------------------------------------
# Function : generate-dir
# Arguments: 1: directory path
# Returns  : Generate a rule, but not dependency, to create a directory with
#            host-mkdir.
# Usage    : $(call generate-dir,<path>)
# -----------------------------------------------------------------------------
define ev-generate-dir
__build_dir := $1
ifeq (,$$(__build_dir_flag__$$(__build_dir)))
# Note that the following doesn't work because path in windows may contain
# ':' if ndk-build is called inside jni/ directory when path is expanded
# to full-path, eg. C:/path/to/project/jni/
#
#    __build_dir_flag__$1 := true
#
__build_dir_flag__$$(__build_dir) := true
$1:
	@$$(call host-mkdir,$$@)
endif
endef

generate-dir = $(eval $(call ev-generate-dir,$1))

# -----------------------------------------------------------------------------
# Function : generate-file-dir
# Arguments: 1: file path
# Returns  : Generate a dependency and a rule to ensure that the parent
#            directory of the input file path will be created before it.
#            This is used to enforce a call to host-mkdir.
# Usage    : $(call generate-file-dir,<file>)
# Rationale: Many object files will be stored in the same output directory.
#            Introducing a dependency on the latter avoids calling mkdir -p
#            for every one of them.
#
# -----------------------------------------------------------------------------

define ev-generate-file-dir
__build_file_dir := $(call parent-dir,$1)
$$(call generate-dir,$$(__build_file_dir))
$1:| $$(__build_file_dir)
endef

generate-file-dir = $(eval $(call ev-generate-file-dir,$1))

# -----------------------------------------------------------------------------
# Function : generate-list-file
# Arguments: 1: list of strings (possibly very long)
#            2: file name
# Returns  : write the content of a possibly very long string list to a file.
#            this shall be used in commands and will work around limitations
#            of host command-line lengths.
# Usage    : $(call host-echo-to-file,<string-list>,<file>)
# Rationale: When there is a very large number of objects and/or libraries at
#            link time, the size of the command becomes too large for the
#            host system's maximum. Various tools however support the
#            @<listfile> syntax, where <listfile> is the path of a file
#            which content will be parsed as if they were options.
#
#            This function is used to generate such a list file from a long
#            list of strings in input.
#
# -----------------------------------------------------------------------------

# Helper functions because the GNU Make $(word ...) function does
# not accept a 0 index, so we need to bump any of these to 1 when
# we find them.
#
index-is-zero = $(filter 0 00 000 0000 00000 000000 0000000,$1)
bump-0-to-1 = $(if $(call index-is-zero,$1),1,$1)

-test-bump-0-to-1 = \
  $(call test-expect,$(call bump-0-to-1))\
  $(call test-expect,1,$(call bump-0-to-1,0))\
  $(call test-expect,1,$(call bump-0-to-1,1))\
  $(call test-expect,2,$(call bump-0-to-1,2))\
  $(call test-expect,1,$(call bump-0-to-1,00))\
  $(call test-expect,1,$(call bump-0-to-1,000))\
  $(call test-expect,1,$(call bump-0-to-1,0000))\
  $(call test-expect,1,$(call bump-0-to-1,00000))\
  $(call test-expect,1,$(call bump-0-to-1,000000))\
  $(call test-expect,10,$(call bump-0-to-1,10))\
  $(call test-expect,100,$(call bump-0-to-1,100))

# Same as $(wordlist ...) except the start index, if 0, is bumped to 1
index-word-list = $(wordlist $(call bump-0-to-1,$1),$2,$3)

-test-index-word-list = \
  $(call test-expect,,$(call index-word-list,1,1))\
  $(call test-expect,a b,$(call index-word-list,0,2,a b c d))\
  $(call test-expect,b c,$(call index-word-list,2,3,a b c d))\

# NOTE: With GNU Make $1 and $(1) are equivalent, which means
#       that $10 is equivalent to $(1)0, and *not* $(10).

# Used to generate a slice of up to 10 items starting from index $1,
# If $1 is 0, it will be bumped to 1 (and only 9 items will be printed)
# $1: start (tenth) index. Can be 0
# $2: word list
#
define list-file-start-gen-10
	$$(hide) $$(BUILD_HOST_ECHO_N) "$(call index-word-list,$10,$19,$2) " >> $$@
endef

# Used to generate a slice of always 10 items starting from index $1
# $1: start (tenth) index. CANNOT BE 0
# $2: word list
define list-file-always-gen-10
	$$(hide) $$(BUILD_HOST_ECHO_N) "$(wordlist $10,$19,$2) " >> $$@
endef

# Same as list-file-always-gen-10, except that the word list might be
# empty at position $10 (i.e. $(1)0)
define list-file-maybe-gen-10
ifneq ($(word $10,$2),)
	$$(hide) $$(BUILD_HOST_ECHO_N) "$(wordlist $10,$19,$2) " >> $$@
endif
endef

define list-file-start-gen-100
$(call list-file-start-gen-10,$10,$2)
$(call list-file-always-gen-10,$11,$2)
$(call list-file-always-gen-10,$12,$2)
$(call list-file-always-gen-10,$13,$2)
$(call list-file-always-gen-10,$14,$2)
$(call list-file-always-gen-10,$15,$2)
$(call list-file-always-gen-10,$16,$2)
$(call list-file-always-gen-10,$17,$2)
$(call list-file-always-gen-10,$18,$2)
$(call list-file-always-gen-10,$19,$2)
endef

define list-file-always-gen-100
$(call list-file-always-gen-10,$10,$2)
$(call list-file-always-gen-10,$11,$2)
$(call list-file-always-gen-10,$12,$2)
$(call list-file-always-gen-10,$13,$2)
$(call list-file-always-gen-10,$14,$2)
$(call list-file-always-gen-10,$15,$2)
$(call list-file-always-gen-10,$16,$2)
$(call list-file-always-gen-10,$17,$2)
$(call list-file-always-gen-10,$18,$2)
$(call list-file-always-gen-10,$19,$2)
endef

define list-file-maybe-gen-100
ifneq ($(word $(call bump-0-to-1,$100),$2),)
ifneq ($(word $199,$2),)
$(call list-file-start-gen-10,$10,$2)
$(call list-file-always-gen-10,$11,$2)
$(call list-file-always-gen-10,$12,$2)
$(call list-file-always-gen-10,$13,$2)
$(call list-file-always-gen-10,$14,$2)
$(call list-file-always-gen-10,$15,$2)
$(call list-file-always-gen-10,$16,$2)
$(call list-file-always-gen-10,$17,$2)
$(call list-file-always-gen-10,$18,$2)
$(call list-file-always-gen-10,$19,$2)
else
ifneq ($(word $150,$2),)
$(call list-file-start-gen-10,$10,$2)
$(call list-file-always-gen-10,$11,$2)
$(call list-file-always-gen-10,$12,$2)
$(call list-file-always-gen-10,$13,$2)
$(call list-file-always-gen-10,$14,$2)
$(call list-file-maybe-gen-10,$15,$2)
$(call list-file-maybe-gen-10,$16,$2)
$(call list-file-maybe-gen-10,$17,$2)
$(call list-file-maybe-gen-10,$18,$2)
$(call list-file-maybe-gen-10,$19,$2)
else
$(call list-file-start-gen-10,$10,$2)
$(call list-file-maybe-gen-10,$11,$2)
$(call list-file-maybe-gen-10,$12,$2)
$(call list-file-maybe-gen-10,$13,$2)
$(call list-file-maybe-gen-10,$14,$2)
endif
endif
endif
endef

define list-file-maybe-gen-1000
ifneq ($(word $(call bump-0-to-1,$1000),$2),)
ifneq ($(word $1999,$2),)
$(call list-file-start-gen-100,$10,$2)
$(call list-file-always-gen-100,$11,$2)
$(call list-file-always-gen-100,$12,$2)
$(call list-file-always-gen-100,$13,$2)
$(call list-file-always-gen-100,$14,$2)
$(call list-file-always-gen-100,$15,$2)
$(call list-file-always-gen-100,$16,$2)
$(call list-file-always-gen-100,$17,$2)
$(call list-file-always-gen-100,$18,$2)
$(call list-file-always-gen-100,$19,$2)
else
ifneq ($(word $1500,$2),)
$(call list-file-start-gen-100,$10,$2)
$(call list-file-always-gen-100,$11,$2)
$(call list-file-always-gen-100,$12,$2)
$(call list-file-always-gen-100,$13,$2)
$(call list-file-always-gen-100,$14,$2)
$(call list-file-maybe-gen-100,$15,$2)
$(call list-file-maybe-gen-100,$16,$2)
$(call list-file-maybe-gen-100,$17,$2)
$(call list-file-maybe-gen-100,$18,$2)
$(call list-file-maybe-gen-100,$19,$2)
else
$(call list-file-start-gen-100,$10,$2)
$(call list-file-maybe-gen-100,$11,$2)
$(call list-file-maybe-gen-100,$12,$2)
$(call list-file-maybe-gen-100,$13,$2)
$(call list-file-maybe-gen-100,$14,$2)
endif
endif
endif
endef


define generate-list-file-ev
__list_file := $2

.PHONY: $$(__list_file).tmp

$$(call generate-file-dir,$$(__list_file).tmp)

$$(__list_file).tmp:
	$$(hide) $$(BUILD_HOST_ECHO_N) "" > $$@
$(call list-file-maybe-gen-1000,0,$1)
$(call list-file-maybe-gen-1000,1,$1)
$(call list-file-maybe-gen-1000,2,$1)
$(call list-file-maybe-gen-1000,3,$1)
$(call list-file-maybe-gen-1000,4,$1)
$(call list-file-maybe-gen-1000,5,$1)

$$(__list_file): $$(__list_file).tmp
	$$(hide) $$(call host-copy-if-differ,$$@.tmp,$$@)
	$$(hide) $$(call host-rm,$$@.tmp)

endef

generate-list-file = $(eval $(call generate-list-file-ev,$1,$2))


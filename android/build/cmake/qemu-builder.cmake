# Copyright 2018 The Android Open Source Project
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


# This function retrieves th git_version, or reverting to the date if
# we cannot fetch it.
#

function(complete_archive LIBCMD LIBNAME)
if(APPLE OR ANDROID_TARGET_TAG STREQUAL "darwin-x86_64")
    set(${LIBCMD} "-Wl,-force_load," ${LIBNAME} PARENT_SCOPE)
else()
    set(${LIBCMD} "-Wl,-whole-archive"
        ${LIBNAME}
        "-Wl,-no-whole-archive" PARENT_SCOPE)
endif()
endfunction()

function(create_qemu_target AARCH LOCAL_AARCH STUBS CPU LAST_LIBS)
    complete_archive(QEMU_SHARED_LIB "qemu2-shared")
    add_executable(qemu-system-${AARCH}
        android-qemu2-glue/main.cpp
        vl.c
        ${STUBS}
        ${qemu-system-${AARCH}_sources}
        ${qemu-system-${AARCH}_generated_sources})
    target_include_directories(qemu-system-${AARCH}
        PRIVATE android-qemu2-glue/config/target-${LOCAL_AARCH}
        target/${CPU})
    target_compile_definitions(qemu-system-${AARCH}
        PRIVATE
        -DNEED_CPU_H
        -DCONFIG_ANDROID)
    target_link_libraries(qemu-system-${AARCH}
        PRIVATE android-qemu-deps
        -w
        ${QEMU_SHARED_LIB}
        qemuutil
        emulator-libui
        android-emu
        libqemu2-glue
        libOpenGLESDispatch
        ${LAST_LIBS})
    # Setup the custom destination directories, so we are binplaced properly
    # after building.
    set_target_properties(qemu-system-${AARCH}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qemu/${ANDROID_TARGET_TAG}"
        )
    target_prebuilt_dependency(qemu-system-${AARCH} "${RUNTIME_OS_DEPENDENCIES}"
        "${RUNTIME_OS_PROPERTIES}")
endfunction()

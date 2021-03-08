#-------------------------------------------------------------------------
# drawElements CMake utilities
# ----------------------------
#
# Copyright 2016 The Android Open Source Project
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
#-------------------------------------------------------------------------

# Android Emulator Target

set(DEQP_TARGET_NAME	"AEMU")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -DANDROID ${AEMU_SANITIZER}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -DANDROID -std=c++11 ${AEMU_SANITIZER}" CACHE STRING "" FORCE)

include_directories(${GOLDFISH_OPENGL_INCLUDE_DIR})
include_directories(${AEMU_INCLUDE_DIR})

# GLESv1 lib
find_library(GLES1_LIBRARY GLESv1_CM_emulation PATHS ${AEMU_LIBRARY_DIR})
set(DEQP_GLES1_LIBRARIES ${GLES1_LIBRARY})

# GLESv2 lib
find_library(GLES2_LIBRARY GLESv2_emulation PATHS ${AEMU_LIBRARY_DIR})
set(DEQP_GLES2_LIBRARIES ${GLES2_LIBRARY})

# GLESv3 lib
find_library(GLES3_LIBRARY GLESv2_emulation PATHS ${AEMU_LIBRARY_DIR})
set(DEQP_GLES3_LIBRARIES ${GLES2_LIBRARY})


# GLESv31 lib
find_library(GLES31_LIBRARY GLESv2_emulation PATHS ${AEMU_LIBRARY_DIR})
set(DEQP_GLES31_LIBRARIES ${GLES2_LIBRARY})

# EGL lib
find_library(EGL_LIBRARY EGL_emulation PATHS ${AEMU_LIBRARY_DIR})
set(DEQP_EGL_LIBRARIES ${EGL_LIBRARY})

# Platform libs
find_library(AEMUGRAPHICS_LIBRARY libaemugraphics aemugraphics PATHS ${AEMU_LIBRARY_DIR})
set(DEQP_PLATFORM_LIBRARIES ${DEQP_PLATFORM_LIBRARIES} ${AEMUGRAPHICS_LIBRARY})

# Platform sources
set(TCUTIL_PLATFORM_SRCS
    aemu/tcuAEMUPlatform.cpp
	aemu/tcuAEMUPlatform.hpp
	tcuMain.cpp
	)

include_directories(${OPENGL_INCLUDE_DIRS})

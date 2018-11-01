# Copyright 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
# specific language governing permissions and limitations under the License.

# The names here come from the cmake swiftshader project. In theory you should be able
# to include that over this prebuilt

get_filename_component(
  PREBUILT_ROOT
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/swiftshader/${ANDROID_TARGET_TAG}/lib/" ABSOLUTE)

if(NOT TARGET libEGL)
    add_library(libEGL SHARED IMPORTED GLOBAL)
    # You can define two import-locations: one for debug and one for release.
    set_target_properties(libEGL PROPERTIES 
                                IMPORTED_LOCATION 
                                "${PREBUILT_ROOT}/libEGL${CMAKE_SHARED_LIBRARY_SUFFIX}")

    add_library(libGLESv2 SHARED IMPORTED GLOBAL)

    set_target_properties(libGLESv2 PROPERTIES 
                                IMPORTED_LOCATION 
                                "${PREBUILT_ROOT}/libGLESv2${CMAKE_SHARED_LIBRARY_SUFFIX}")


    add_library(libGLES_CM SHARED IMPORTED GLOBAL)

    set_target_properties(libGLES_CM PROPERTIES 
                                    IMPORTED_LOCATION 
                                    "${PREBUILT_ROOT}/libGLES_CM${CMAKE_SHARED_LIBRARY_SUFFIX}")
                            

endif()

# Note that this should automatically pick up the swiftshader targets if they were imported
# from the original swiftshader cmakelists.txt
if(NOT TARGET Swiftshader)
   add_custom_target(Swiftshader
   COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libGLESv2> ${CMAKE_BINARY_DIR}/lib64/gles_swiftshader
   COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libEGL> ${CMAKE_BINARY_DIR}/lib64/gles_swiftshader
   COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libGLES_CM> ${CMAKE_BINARY_DIR}/lib64/gles_swiftshader
   )
   add_dependencies(Swiftshader libEGL libGLESv2 libGLES_CM)
endif()

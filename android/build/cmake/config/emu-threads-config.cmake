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

# We now what kinds of threads we are using, and we've disabled finding of packages, so we just declare it explicitly
if (NOT TARGET Threads::Threads)
  add_library(Threads::Threads INTERFACE IMPORTED)
endif()

if(WINDOWS_X86_64)
  # Let's statically link the mingw thread library.. (Leaving out -lstdc++ will make it dynamic)
  set_property(TARGET Threads::Threads PROPERTY INTERFACE_LINK_LIBRARIES "-Wl,-Bstatic;-lstdc++;-lwinpthread;-Wl,-Bdynamic")
else()
  set_property(TARGET Threads::Threads PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
  set_property(TARGET Threads::Threads PROPERTY INTERFACE_LINK_LIBRARIES "-lpthread")
endif()
set(CMAKE_USE_PTHREADS_INIT 1)
set(Threads_FOUND TRUE)
set(PACKAGE_EXPORT "Threads_FOUND;CMAKE_USE_PTHREADS_INIT")

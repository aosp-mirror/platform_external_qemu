
android_add_library(TARGET android-emu-tracing SHARED LICENSE Apache-2.0
                    SRC android/base/Tracing.cpp
                    DEPS android-emu-base-headers)
android_install_shared_library(TARGET android-emu-tracing)

if(WINDOWS_MSVC_X86_64)
  target_compile_definitions(android-emu-tracing PUBLIC AEMU_TRACING_SHARED
                                                 PRIVATE TRACING_EXPORTS)
endif()

target_link_libraries(android-emu-tracing PRIVATE perfetto-tracing-only protobuf::libprotobuf)
target_include_directories(android-emu-tracing PUBLIC .)

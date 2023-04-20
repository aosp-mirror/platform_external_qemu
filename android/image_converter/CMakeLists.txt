android_add_library(TARGET image_converter SHARED LICENSE Apache-2.0
                    SRC ImageConverter.c)

if(LINUX_X86_64 OR DARWIN_X86_64 OR WINDOWS_MSVC_X86_64)
  target_compile_options(image_converter PRIVATE "-march=haswell")
endif()

target_include_directories(image_converter PRIVATE .
                                                   ${CMAKE_CURRENT_BINARY_DIR})
android_install_shared(image_converter)

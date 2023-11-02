android_add_library(TARGET android-emu-curl SHARED LICENSE Apache-2.0 SOURCE_DIR
                    src SRC android/curl-support.c DEPS android-emu-base curl ssl crypto)
target_include_directories(android-emu-curl PUBLIC include PRIVATE src)
target_compile_definitions(android-emu-curl PRIVATE AEMU_CURL_EXPORTS=1)
target_link_libraries(android-emu-curl PRIVATE android-emu-base-headers android-emu-base-logging)
android_install_shared_library(TARGET android-emu-curl)

set(aemugraphics_src ClientComposer.cpp Display.cpp GoldfishOpenglTestEnv.cpp Toplevel.cpp window-agent-mock-impl.cpp)
android_add_shared_library(aemugraphics)
target_compile_options(aemugraphics PRIVATE -fvisibility=default)

# Without this you will pick up the X11 libs when including eglplatform.h
# which will result into compile issues on linux as the X11 libs will get picked up.
target_compile_definitions(aemugraphics PUBLIC -DANDROID=1)
target_include_directories(aemugraphics
                                   PRIVATE
                                   ${GOLDFISH_EMUGL_DIR}/host/include/libOpenglRender
                                   ${GOLDFISH_EMUGL_DIR}/system/include
                                   ${ANDROID_EMUGL_DIR}/guest
                                   ${ANDROID_EMUGL_DIR}/guest/androidImpl
                                   ${ANDROID_EMUGL_DIR}/host/include
                                   ${ANDROID_EMUGL_DIR}/host/libs/libOpenglRender/standalone_common/angle-util)
target_compile_options(aemugraphics PRIVATE -fvisibility=default)
target_link_libraries(aemugraphics
                              PUBLIC
                              OSWindow
                              android-emu-shared
                              cutils
                              gui
                              OpenglSystemCommon
                              EGL_emulation
                              GLESv2_emulation
                              emugl_base
                              OpenglRender_standalone_common)
android_target_link_libraries(aemugraphics darwin-x86_64 PUBLIC "-framework CoreFoundation"
                              "-framework CoreGraphics")

# Combined unit tests
set(emugl_combined_unittests_src combined_unittest.cpp)
android_add_test(emugl_combined_unittests)
target_link_libraries(emugl_combined_unittests
                              PUBLIC
                              aemugraphics
                              OSWindow
                              gtest
                              gtest_main)
target_compile_definitions(emugl_combined_unittests PRIVATE -D__ANDROID_API__=28)
target_include_directories(emugl_combined_unittests PRIVATE 
                                   ${GOLDFISH_EMUGL_DIR}/host/include/libOpenglRender
                                   ${GOLDFISH_EMUGL_DIR}/system/include
                                   ${ANDROID_EMUGL_DIR}/guest/androidImpl)

# These are usually nops, but some code generators will not automatically binplace these (Xcode!)
# So let's just guarantee that it gets properly binplaced
android_copy_shared_lib(emugl_combined_unittests EGL_translator EGL_translator)
android_copy_shared_lib(emugl_combined_unittests GLES_V2_translator GLES_V2_translator)
android_copy_shared_lib(emugl_combined_unittests GLES_CM_translator GLES_CM_translator)
android_copy_shared_lib(emugl_combined_unittests GLES12Translator GLES12Translator)
android_copy_shared_lib(emugl_combined_unittests OpenglRender OpenglRender)
android_copy_shared_lib(emugl_combined_unittests gralloc.ranchu gralloc.ranchu)
android_copy_shared_lib(emugl_combined_unittests gralloc.goldfish gralloc.goldfish)

android_target_dependency(emugl_combined_unittests all  EMULATOR_FEATURE_DEPENDENCIES)
android_target_dependency(emugl_combined_unittests all  SWIFTSHADER_DEPENDENCIES)
android_target_properties(emugl_combined_unittests darwin-x86_64 "INSTALL_RPATH>=@loader_path/lib64/gles_swiftshader")
android_target_properties(emugl_combined_unittests linux-x86_64 "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/gles_swiftshader'")

# Toplevel unit tests
set(aemugraphics_toplevel_unittests_src toplevel_unittest.cpp)
android_add_test(aemugraphics_toplevel_unittests)
target_compile_options(aemugraphics_toplevel_unittests PRIVATE -fvisibility=default)
target_compile_definitions(aemugraphics_toplevel_unittests PRIVATE -D__ANDROID_API__=28)
target_include_directories(aemugraphics_toplevel_unittests PRIVATE 
                                   ${GOLDFISH_EMUGL_DIR}/host/include/libOpenglRender
                                   ${GOLDFISH_EMUGL_DIR}/system/include)
target_link_libraries(aemugraphics_toplevel_unittests
                              PUBLIC
                              aemugraphics
                              OSWindow
                              gtest
                              gtest_main)
android_target_dependency(aemugraphics_toplevel_unittests all  SWIFTSHADER_DEPENDENCIES)
android_target_dependency(aemugraphics_toplevel_unittests all  EMULATOR_FEATURE_DEPENDENCIES)
android_target_properties(aemugraphics_toplevel_unittests darwin-x86_64 "INSTALL_RPATH>=@loader_path/lib64/gles_swiftshader")
android_target_properties(aemugraphics_toplevel_unittests linux-x86_64 "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/gles_swiftshader'")

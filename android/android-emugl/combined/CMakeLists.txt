set(aemugraphics_src ClientComposer.cpp Display.cpp Toplevel.cpp)
android_add_shared_library(aemugraphics)
android_target_link_libraries(aemugraphics
                              all
                              PUBLIC
                              OSWindow
                              android-emu-base
                              cutils
                              gui
                              OpenglSystemCommon
                              EGL_emulation
                              GLESv2_emulation
                              emugl_base
                              goldfish)
android_target_link_libraries(aemugraphics darwin-x86_64 PUBLIC "-framework CoreFoundation"
                              "-framework CoreGraphics")

# Combined unit tests
set(emugl_combined_unittests_src combined_unittest.cpp)
android_add_test(emugl_combined_unittests)
android_target_include_directories(aemugraphics
                                   all
                                   PRIVATE
                                   ${GOLDFISH_EMUGL_DIR}/host/include/libOpenglRender
                                   ${GOLDFISH_EMUGL_DIR}/system/include
                                   ${ANDROID_EMUGL_DIR}/guest
                                   ${ANDROID_EMUGL_DIR}/guest/androidImpl
                                   ${ANDROID_EMUGL_DIR}/host/include
                                   ${ANDROID_EMUGL_DIR}/host/libs/libOpenglRender/standalone_common/angle-util)
android_target_compile_options(aemugraphics all PRIVATE -fvisibility=default)

android_target_link_libraries(emugl_combined_unittests
                              all
                              PUBLIC
                              aemugraphics
                              OSWindow
                              android-emu-base
                              gtest
                              gtest_main)
android_target_include_directories(emugl_combined_unittests all PRIVATE ${ANDROID_EMUGL_DIR}/guest/androidImpl)

# These are usually nops, but some code generators will not automatically binplace these (Xcode!)
android_copy_shared_lib(emugl_combined_unittests EGL_translator lib${ANDROID_TARGET_BITS}EGL_translator)
android_copy_shared_lib(emugl_combined_unittests GLES_V2_translator lib${ANDROID_TARGET_BITS}GLES_V2_translator)
android_copy_shared_lib(emugl_combined_unittests GLES_CM_translator lib${ANDROID_TARGET_BITS}GLES_CM_translator)
android_copy_shared_lib(emugl_combined_unittests GLES12Translator lib${ANDROID_TARGET_BITS}GLES12Translator)
android_copy_shared_lib(emugl_combined_unittests OpenglRender lib${ANDROID_TARGET_BITS}OpenglRender)
android_copy_shared_lib(emugl_combined_unittests gralloc.ranchu gralloc.ranchu)

android_target_dependency(emugl_combined_unittests all "${EMULATOR_FEATURE_DEPENDENCIES}")
android_target_dependency(emugl_combined_unittests all "${SWIFTSHADER_DEPENDENCIES}")
android_target_properties(emugl_combined_unittests darwin-x86_64 "INSTALL_RPATH>=@loader_path/lib64/gles_swiftshader")
android_target_properties(emugl_combined_unittests linux-x86_64 "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/gles_swiftshader'")

# Toplevel unit tests
set(aemugraphics_toplevel_unittests_src toplevel_unittest.cpp)
android_add_test(aemugraphics_toplevel_unittests)
android_target_compile_options(aemugraphics_toplevel_unittests all PRIVATE -fvisibility=default)
android_target_link_libraries(aemugraphics_toplevel_unittests
                              all
                              PUBLIC
                              aemugraphics
                              OSWindow
                              android-emu-base
                              gtest
                              gtest_main)
android_target_dependency(aemugraphics_toplevel_unittests all "${SWIFTSHADER_DEPENDENCIES}")
android_target_dependency(aemugraphics_toplevel_unittests all "${EMULATOR_FEATURE_DEPENDENCIES}")
android_target_properties(aemugraphics_toplevel_unittests darwin-x86_64 "INSTALL_RPATH>=@loader_path/lib64/gles_swiftshader")
android_target_properties(aemugraphics_toplevel_unittests linux-x86_64 "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/gles_swiftshader'")

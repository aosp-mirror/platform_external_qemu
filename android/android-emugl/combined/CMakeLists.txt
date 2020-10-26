if(OPTION_GFXSTREAM_BACKEND)
  android_add_library(
    TARGET gfxstream_backend
    SHARED
    LICENSE Apache-2.0
    SRC ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/emulation/virtio-goldfish-pipe.cpp
        GfxStreamAgents.cpp
        GfxStreamBackend.cpp)

  target_compile_options(gfxstream_backend PRIVATE -fvisibility=hidden)

  target_include_directories(
    gfxstream_backend
    PRIVATE
      ${VIRGLRENDERER_INCLUDE_DIR}
      ${ANDROID_QEMU2_TOP_DIR}/include
      ${ANDROID_QEMU2_TOP_DIR}/qemu2-auto-generated
      ${GOLDFISH_EMUGL_DIR}/host/include/libOpenglRender
      ${GOLDFISH_EMUGL_DIR}/system/include
      ${ANDROID_EMUGL_DIR}/guest
      ${ANDROID_EMUGL_DIR}/guest/androidImpl
      ${ANDROID_EMUGL_DIR}/host/include
      ${ANDROID_EMUGL_DIR}/host/libs/libOpenglRender/standalone_common/angle-util
  )

  android_target_properties(gfxstream_backend linux-x86_64
                            "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN'")
  android_target_properties(gfxstream_backend darwin-x86_64
                            "INSTALL_RPATH>=@loader_path")

  if(NOT LINUX_AARCH64)
    target_link_libraries(
      gfxstream_backend PRIVATE GLIB2::GLIB2 OSWindow android-emu-shared
                                emugl_base OpenglRender)
  else()
    target_link_libraries(
      gfxstream_backend PRIVATE GLIB2::GLIB2 android-emu-shared emugl_base
                                OpenglRender)
  endif()

  if(NOT LINUX_AARCH64)
    android_target_dependency(gfxstream_backend all ANGLE_DEPENDENCIES)
  endif()

  android_install_shared(gfxstream_backend)

  if(NOT LINUX_AARCH64)
    android_install_dependency(all ANGLE_DEPENDENCIES)
  endif()

  if(CROSVM)
    android_crosvm_build(gfxstream_backend)
    set(CMAKE_CROSVM_SRC_PATH "${ANDROID_QEMU2_TOP_DIR}/../../platform/crosvm")
    add_custom_target(
      crosvm_gfxstream ALL
      DEPENDS "${CMAKE_BINARY_DIR}/crosvm-build-env/release/crosvm")
  endif()

  android_add_test(TARGET gfxstream_unittests SRC gfxstream_unittest.cpp)
  target_compile_options(gfxstream_unittests PRIVATE -fvisibility=default)
  target_link_libraries(
    gfxstream_unittests PUBLIC gfxstream_backend android-emu-base gtest
                               gtest_main OSWindow)
  target_include_directories(
    gfxstream_unittests
    PRIVATE
      ${VIRGLRENDERER_INCLUDE_DIR}
      ${ANDROID_QEMU2_TOP_DIR}/include
      ${ANDROID_QEMU2_TOP_DIR}/qemu2-auto-generated
      ${GOLDFISH_EMUGL_DIR}/host/include/libOpenglRender
      ${GOLDFISH_EMUGL_DIR}/system/include
      ${ANDROID_EMUGL_DIR}/guest
      ${ANDROID_EMUGL_DIR}/guest/androidImpl
      ${ANDROID_EMUGL_DIR}/host/include
      ${ANDROID_EMUGL_DIR}/host/libs/libOpenglRender/standalone_common/angle-util
  )

  android_target_properties(
    gfxstream_unittests darwin-x86_64
    "INSTALL_RPATH>=@loader_path/lib64/gles_swiftshader")
  android_target_properties(
    gfxstream_unittests linux-x86_64
    "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/gles_swiftshader'")
endif()

if(NOT WINDOWS AND NOT LINUX_AARCH64)

  set(aemugraphics_src ClientComposer.cpp Display.cpp GoldfishOpenglTestEnv.cpp
                       Toplevel.cpp)
  android_add_library(
    TARGET aemugraphics SHARED LICENSE Apache-2.0 SRC # cmake-format: sortable
                                                      ${aemugraphics_src})
  target_compile_options(aemugraphics PRIVATE -fvisibility=default)

  # Without this you will pick up the X11 libs when including eglplatform.h
  # which will result into compile issues on linux as the X11 libs will get
  # picked up.
  target_compile_definitions(aemugraphics PUBLIC -DANDROID=1)
  target_include_directories(
    aemugraphics
    PRIVATE
      ${GOLDFISH_EMUGL_DIR}/host/include/libOpenglRender
      ${GOLDFISH_EMUGL_DIR}/system/include
      ${ANDROID_EMUGL_DIR}/guest
      ${ANDROID_EMUGL_DIR}/guest/androidImpl
      ${ANDROID_EMUGL_DIR}/host/include
      ${ANDROID_EMUGL_DIR}/host/libs/libOpenglRender/standalone_common/angle-util
  )
  target_compile_options(aemugraphics PRIVATE -fvisibility=default)
  target_link_libraries(
    aemugraphics
    PUBLIC OSWindow
           android-emu-shared
           cutils
           gui
           OpenglSystemCommon
           EGL_emulation
           GLESv2_emulation
           emugl_base
           OpenglRender_standalone_common)
  android_target_link_libraries(
    aemugraphics darwin-x86_64 PUBLIC "-framework CoreFoundation"
                                      "-framework CoreGraphics")

  # Combined unit tests
  android_add_test(TARGET emugl_combined_unittests SRC # cmake-format: sortable
                                                       combined_unittest.cpp)
  target_link_libraries(emugl_combined_unittests
                        PUBLIC aemugraphics OSWindow android-emu-test-launcher)
  target_compile_definitions(emugl_combined_unittests
                             PRIVATE -D__ANDROID_API__=28)
  target_include_directories(
    emugl_combined_unittests
    PRIVATE ${GOLDFISH_EMUGL_DIR}/host/include/libOpenglRender
            ${GOLDFISH_EMUGL_DIR}/system/include
            ${ANDROID_EMUGL_DIR}/guest/androidImpl)

  # These are usually nops, but some code generators will not automatically
  # binplace these (Xcode!) So let's just guarantee that it gets properly
  # binplaced
  android_copy_shared_lib(emugl_combined_unittests OpenglRender OpenglRender)
  android_copy_shared_lib(emugl_combined_unittests gralloc.ranchu
                          gralloc.ranchu)
  android_copy_shared_lib(emugl_combined_unittests gralloc.goldfish
                          gralloc.goldfish)

  android_target_dependency(emugl_combined_unittests all
                            EMULATOR_FEATURE_DEPENDENCIES)
  android_target_dependency(emugl_combined_unittests all
                            SWIFTSHADER_DEPENDENCIES)
  android_target_properties(
    emugl_combined_unittests darwin-x86_64
    "INSTALL_RPATH>=@loader_path/lib64/gles_swiftshader")
  android_target_properties(
    emugl_combined_unittests linux-x86_64
    "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/gles_swiftshader'")

  # Toplevel unit tests
  android_add_test(
    TARGET aemugraphics_toplevel_unittests SRC # cmake-format: sortable
                                               toplevel_unittest.cpp)
  target_compile_options(aemugraphics_toplevel_unittests
                         PRIVATE -fvisibility=default)
  target_compile_definitions(aemugraphics_toplevel_unittests
                             PRIVATE -D__ANDROID_API__=28)
  target_include_directories(
    aemugraphics_toplevel_unittests
    PRIVATE ${GOLDFISH_EMUGL_DIR}/host/include/libOpenglRender
            ${GOLDFISH_EMUGL_DIR}/system/include)
  target_link_libraries(
    aemugraphics_toplevel_unittests PUBLIC aemugraphics OSWindow gtest gmock
                                           android-emu-test-launcher)
  android_target_dependency(aemugraphics_toplevel_unittests all
                            SWIFTSHADER_DEPENDENCIES)
  android_target_dependency(aemugraphics_toplevel_unittests all
                            EMULATOR_FEATURE_DEPENDENCIES)
  android_target_properties(
    aemugraphics_toplevel_unittests darwin-x86_64
    "INSTALL_RPATH>=@loader_path/lib64/gles_swiftshader")
  android_target_properties(
    aemugraphics_toplevel_unittests linux-x86_64
    "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/gles_swiftshader'")

  # Vulkan encoder unit tests
  android_add_test(
    TARGET vulkan_enc_unittests
    SRC # cmake-format: sortable
        ${GOLDFISH_EMUGL_DIR}/system/vulkan_enc/vulkan_enc_unittests.cpp)
  target_include_directories(vulkan_enc_unittests
                             PRIVATE ${GOLDFISH_EMUGL_DIR}/android-emu)
  target_link_libraries(vulkan_enc_unittests PUBLIC vulkan_enc androidemu gtest
                                                    gtest_main)
endif()

if(OPTION_GFXSTREAM_BACKEND)
    if (WIN32)
        add_definitions("-DUNICODE -D_UNICODE -DNOMINMAX -DEMUGL_BUILD -DVK_USE_PLATFORM_WIN32_KHR -DBUILDING_EMUGL_COMMON_SHARED")

        option(RECORDER_DELEGATE_LIB "Use recorder_delegate_lib dll" OFF)
        if(RECORDER_DELEGATE_LIB)
            add_definitions(-DRECORDER_DELEGATE_LIB)
        endif()
    endif()

    option(VIRGL_RENDERER_UNSTABLE_APIS "Use unstable virglrenderer APIs" OFF)
    if(VIRGL_RENDERER_UNSTABLE_APIS)
        add_definitions(-DVIRGL_RENDERER_UNSTABLE_APIS)
    endif()

    if(UNIX AND NOT APPLE)
        set(LINUX TRUE)
    endif()

    find_package(Threads)
    include(ExternalProject)

    include(GoogleTest)
    enable_testing()
    # Disable test discovery after build.
    # By default, `gtest_discover_tests()` adds a post-build step to run the test executables in order to discover the test
    # targets. This is problematic in some build environments. (for example: if cross-compiling)
    set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE "PRE_TEST")

    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_C_STANDARD 11)

    if (APPLE)
        add_compile_definitions(VK_USE_PLATFORM_MACOS_MVK)
    elseif(UNIX)
        # TODO(kaiyili, b/179477624): Add Linux specific Vulkan platform macro definitions
    elseif(WIN32)
        add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
    endif()

    add_compile_definitions(GLM_FORCE_RADIANS)
    add_compile_definitions(GLM_FORCE_DEFAULT_ALIGNED_GENTYPES)

    if (MSVC)
        # ask msvc not to warn not secure C ISO functions
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
        # ask msvc not to warn non C ISO POSIX functions
        add_compile_definitions(_CRT_NONSTDC_NO_DEPRECATE)
    endif()

    # Macro to easily set the TEST_INCLUDE_FILES properties to point to `test_properties.cmake`
    # This macro should be called at the end of any CMakeLists.txt file that defines test targets.
    macro("set_test_include_files")
    set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES ${CMAKE_SOURCE_DIR}/test_properties.cmake)
    endmacro()

    # Uncomment for ASAN support
    # set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
    # set (CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address")

    if (WIN32)
        if (BUILD_ASAN_WIN32)
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
            # ASAN does not work with flag /MDd, replace it with /MD
            string(REPLACE "/MDd" "/MD" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
            set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})

            # ASAN linker
            # User needs to use -D ASAN_LIB_DIR:STRING=/path/to/asan_libs to add library directory
            if (NOT DEFINED ASAN_LIB_DIR)
                message(FATAL_ERROR "Please input ASAN library path with -D ASAN_LIB_DIR:STRING=/path/to/asan_lib_dir")
            endif()
            link_libraries(clang_rt.asan_dynamic-x86_64.lib clang_rt.asan_dynamic_runtime_thunk-x86_64.lib)
            message("Linking ASAN libraries from: ${ASAN_LIB_DIR}")
            link_directories(${ASAN_LIB_DIR})
        endif()
    endif()

    set(GFXSTREAM_REPO_ROOT ${ANDROID_QEMU2_TOP_DIR}/../../device/generic/vulkan-cereal)
    set(GFXSTREAM_PREFIX "gfxstream")

    # Third party dependencies
    add_subdirectory(${GFXSTREAM_REPO_ROOT}/third-party/angle ${GFXSTREAM_PREFIX}-third-party-angle)
    add_subdirectory(${GFXSTREAM_REPO_ROOT}/third-party/renderdoc ${GFXSTREAM_PREFIX}-third-party-renderdoc)

    # Common base libraries for host################################################

    add_subdirectory(${GFXSTREAM_REPO_ROOT}/base ${GFXSTREAM_PREFIX}-base)
    add_subdirectory(${GFXSTREAM_REPO_ROOT}/snapshot ${GFXSTREAM_PREFIX}-snapshot)
    add_subdirectory(${GFXSTREAM_REPO_ROOT}/host-common ${GFXSTREAM_PREFIX}-host-common)

    # Backends######################################################################

    add_subdirectory(${GFXSTREAM_REPO_ROOT}/stream-servers ${GFXSTREAM_PREFIX}-stream-servers)

    # Protocols and associated code generators######################################

    add_subdirectory(${GFXSTREAM_REPO_ROOT}/protocols ${GFXSTREAM_PREFIX}-protocols)

    # Fake Android guest#########################3##################################

    if (NOT WIN32)
        add_subdirectory(${GFXSTREAM_REPO_ROOT}/fake-android-guest ${GFXSTREAM_PREFIX}-fake-android-guest)
    endif()

    # TODO(joshuaduong): For some reason, our custom rpath is only applied to gfxstream_backend_unittests.
    # for the rest of the unittests below, the rpath is not set, and thus the tests cannot find libc++.
    android_target_properties(gfxstream-base_unittests all "${RUNTIME_OS_PROPERTIES}")
    android_target_properties(gfxstream-host-common_unittests all "${RUNTIME_OS_PROPERTIES}")
    android_target_properties(Vulkan_unittests all "${RUNTIME_OS_PROPERTIES}")
    android_target_properties(OpenglRender_unittests all "${RUNTIME_OS_PROPERTIES}")
    android_target_properties(OpenglRender_snapshot_unittests all "${RUNTIME_OS_PROPERTIES}")

    set(GFXSTREAM_DEPENDENCIES
        angle_common
        angle_shader_translator
        apigen-codec-common
        EGL_translator_static
        gfxstream-base
        gfxstream-compressedTextures
        gfxstream-host-common
        gfxstream-snapshot
        gfxstream-vulkan-server
        gfxstream_backend
        gfxstream_backend_static
        gles1_dec
        gles2_dec
        GLcommon
        GLSnapshot
        GLES_CM_translator_static
        GLES_V2_translator_static
        logging-base
        OpenGLESDispatch
        OpenglRender_vulkan_cereal
        renderControl_dec
        renderdoc)
   android_license(TARGET "${GFXSTREAM_DEPENDENCIES}"
                   LIBNAME gfxstream
                   URL "https://android.googlesource.com/platform/device/generic/vulkan-cereal/+/refs/heads/master"
                   SPDX "Apache-2.0"
                   LOCAL None
                   LICENSE None)
endif()

# TODO(joshuaduong): Make it work for gfxstream build.
if(NOT OPTION_GFXSTREAM_BACKEND AND NOT WINDOWS AND NOT BUILDING_FOR_AARCH64)

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
    emugl_combined_unittests darwin
    "INSTALL_RPATH>=@loader_path/lib64/gles_swiftshader")
  android_target_properties(
    emugl_combined_unittests linux
    "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/gles_swiftshader' -Wl,--disable-new-dtags")

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
    aemugraphics_toplevel_unittests darwin
    "INSTALL_RPATH>=@loader_path/lib64/gles_swiftshader")
  android_target_properties(
    aemugraphics_toplevel_unittests linux
    "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/gles_swiftshader'  -Wl,--disable-new-dtags")

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

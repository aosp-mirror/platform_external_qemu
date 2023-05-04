# The list of dependencies.
prebuilt(QT5)
message(STATUS "Building UI using Qt ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}")

# ~~~
# ! android_qt_lib: Create a target with the given sources and dependencies
#   it will automatically export the _autogen include directory for all the files
#   for which uic and moc are run.
#
# ``TARGET``    The name of the library
# ``DLLEXPORT``  Define used to mark export/import
# ``SHARED``     Present if this should be a shared library
# ``SRC``        The set of sources
# ``ALIAS``      Alias under which this lib is known
# ``DEPS``       List of private dependencies needed to build this library.
# ~~
function(android_qt_lib)
  # Parse arguments
  set(options SHARED)
  set(oneValueArgs TARGET DLLEXPORT DLLENABLE ALIAS)
  set(multiValueArgs SRC DEPS INCLUDES)
  cmake_parse_arguments(ext "${options}" "${oneValueArgs}" "${multiValueArgs}"
                        ${ARGN})

  if(${ext_SHARED})
    message(FATAL_ERROR "${ext_TARGET} is shared, currently not supported.")
    android_add_library(TARGET ${ext_TARGET} SHARED LICENSE Apache-2.0
                        SOURCE_DIR src SRC ${ext_SRC})
    if(WINDOWS_MSVC_X86_64)
      target_compile_definitions(${ext_TARGET} PUBLIC ${ext_DLLENABLE}
                                 PRIVATE ${ext_DLLEXPORT})
    endif()
  else()
    android_add_library(TARGET ${ext_TARGET} LICENSE Apache-2.0 SOURCE_DIR src
                        SRC ${ext_SRC})
  endif()

  if(ext_INCLUDES)
    target_include_directories(${ext_TARGET} PUBLIC ${ext_INCLUDES})
  endif()
  if(NOT "${ext_ALIAS}" STREQUAL "")
    set_property(GLOBAL APPEND PROPERTY ALIAS_LST
                                        "${ext_ALIAS}|${ext_TARGET}\n")
    add_library(${ext_ALIAS} ALIAS ${ext_TARGET})
  endif()
  target_link_libraries(${ext_TARGET} PRIVATE ${ext_DEPS})
  target_include_directories(
    ${ext_TARGET}
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/${ext_TARGET}_autogen/include include
    PRIVATE src)
  # When using agents we use c-structs which have 0 size in c, but 1 size in
  # c++.
  target_compile_options(${ext_TARGET} PRIVATE -Wno-extern-c-compat)

  if(NOT BUILDING_FOR_AARCH64)
    target_compile_definitions(${ext_TARGET} PRIVATE USE_MMX=1)
    target_compile_options(${ext_TARGET} PRIVATE "-mmmx")
  endif()

  if(OPTION_GFXSTREAM_BACKEND)
    target_compile_definitions(${ext_TARGET} PUBLIC AEMU_GFXSTREAM_BACKEND=1)
  endif()
endfunction()

add_subdirectory(modules/aemu-gl-bridge)
add_subdirectory(modules/aemu-gl-init)
add_subdirectory(modules/aemu-ui-headless)
add_subdirectory(modules/aemu-recording)
add_subdirectory(modules/aemu-ui-common)
add_subdirectory(modules/aemu-ui-widgets)
add_subdirectory(modules/aemu-ui-window)
add_subdirectory(modules/aemu-ui-qt)
add_subdirectory(modules/aemu-ext-pages)
add_subdirectory(modules/aemu-ext-pages-grpc)

list(APPEND android-libui-testdata testdata/mp4/video.mp4)

if(FALSE) # OR NOT LINUX_AARCH64)
  android_add_test(
    TARGET aemu-ui-qt_unittests
    SOURCE_DIR test
    SRC android/emulation/control/ScreenCapturer_unittest.cpp
        android/emulation/control/ScreenCapturer_unittest.h
        android/mp4/MP4Dataset_test.cpp
        android/mp4/MP4Dataset_test.h
        android/mp4/MP4Demuxer_test.cpp
        android/mp4/MP4Demuxer_test.h
        android/mp4/SensorLocationEventProvider_test.cpp
        android/mp4/SensorLocationEventProvider_test.h
        android/mp4/VideoMetadataProvider_test.cpp
        android/mp4/VideoMetadataProvider_test.h
        android/recording/FfmpegRecorder.cpp
        android/recording/FfmpegRecorder.h
        android/recording/test/DummyAudioProducer.cpp
        android/recording/test/DummyAudioProducer.h
        android/recording/test/DummyVideoProducer.cpp
        android/recording/test/DummyVideoProducer.h
        android/recording/test/FfmpegRecorder_unittest.cpp
        android/recording/test/FfmpegRecorder_unittest.h
        android/skin/keycode-buffer_unittest.cpp
        android/skin/keycode-buffer_unittest.h
        android/skin/keycode_unittest.cpp
        android/skin/keycode_unittest.h
        android/skin/qt/native-keyboard-event-handler_unittest.cpp
        android/skin/qt/native-keyboard-event-handler_unittest.h
        android/skin/qt/qtmain_dummy_test.cpp
        android/skin/qt/qtmain_dummy_test.h
        android/skin/rect_unittest.cpp
        android/skin/rect_unittest.h)

  android_copy_test_files(aemu-ui-qt_unittests "${android-libui-testdata}"
                          testdata)

  target_compile_options(aemu-ui-qt_unittests
                         PRIVATE -O0 -UNDEBUG -Wno-deprecated-declarations)

  # Target specific compiler flags for windows
  android_target_compile_options(
    aemu-ui-qt_unittests windows
    PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-Wno-literal-suffix>")

  android_target_dependency(aemu-ui-qt_unittests all QT5_SHARED_DEPENDENCIES)
  android_target_properties(aemu-ui-qt_unittests all "${QT5_SHARED_PROPERTIES}")

  target_link_libraries(
    aemu-ui-qt_unittests PRIVATE aemu-ui-qt android-emu aemu-common
                                 aemu-recording gmock_main)

  android_target_link_libraries(aemu-ui-qt_unittests windows_msvc-x86_64
                                PUBLIC dirent-win32)

endif()

if(WINDOWS_MSVC_X86_64)
  # Qt in windows will call main from win-main v.s. calling qt_main. we have to
  # make a separate launch library to make sure that we do not end up with
  # duplicate main symbols when linking aemu-ui-qt (it used to work do to a
  # cmake linker quirk).
  android_add_library(
    TARGET emulator-winqt-launcher LICENSE Apache-2.0 SOURCE_DIR src
    SRC android/skin/qt/windows-qt-launcher.cpp)
  target_link_libraries(emulator-winqt-launcher PRIVATE android-emu-base)
endif()

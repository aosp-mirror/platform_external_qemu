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

set(QT_VERSION_MAJOR 6)
set(QT_VERSION_MINOR 5)
set(QT_VERSION_PATCH 3)
set(QT_LIB_VERSION ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH})
set(QT_LIBINFIX "AndroidEmu")

# TODO: Remove this once we have -no-window working with absolutely no Qt
# dependencies
if(NOT QTWEBENGINE AND (LINUX_X86_64 OR DARWIN_X86_64 OR DARWIN_AARCH64))
  get_filename_component(
    PREBUILT_ROOT
    "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/${ANDROID_TARGET_TAG}-nowebengine"
    ABSOLUTE)

else()
  get_filename_component(
    PREBUILT_ROOT
    "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/${ANDROID_TARGET_TAG}"
    ABSOLUTE)
endif()

set(CMAKE_AUTOMOC TRUE)
set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTORCC TRUE)
if(LINUX)
  # rcc defaults to using zstd compression, but for an unknown reason, emulator is crashing with
  # this compression algorithm.
  message(STATUS "CMAKE_AUTORCC_OPTIONS before = [${CMAKE_AUTORCC_OPTIONS}]")
  set(CMAKE_AUTORCC_OPTIONS "--compress-algo;zlib")
  message(STATUS "CMAKE_AUTORCC_OPTIONS after = [${CMAKE_AUTORCC_OPTIONS}]")
endif()

# # Use the host compatible moc, rcc and uic if(HOST_DARWIN_X86_64 AND
# DARWIN_AARCH64) # We have Qt6 compiler tools specifically for cross-compiling
# from # darwin-x86_64 to darwin-aarch64 (with QtWebEngine)
# get_filename_component( HOST_PREBUILT_ROOT
# "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/${ANDROID_HOST_TAG}/qt6"
# ABSOLUTE) elseif(HOST_DARWIN_AARCH64 AND DARWIN_X86_64) message(STATUS
# "Reverting to rosetta based Uic.") # We have to revert to using Qt5, and run
# them in precompilaton made.. get_filename_component( HOST_PREBUILT_ROOT
# "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/darwin-x86_64"
# ABSOLUTE) else() get_filename_component( HOST_PREBUILT_ROOT
# "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/${ANDROID_HOST_TAG}"
# ABSOLUTE) endif()

# Registers the uic/rcc/moc targets if needed..
function(add_qt_target target)

  # No need to add existing target..
  if(TARGET Qt${QT_VERSION_MAJOR}::${target})
    return()
  endif()

  if(MSVC)
    set(EXE_SUFFIX ".exe")
  endif()

  # The executable directory has changed from bin --> libexec in Qt6. Windows is still bin.
  if (MSVC)
    set(QT_EXE_DIR "bin")
  else()
    set(QT_EXE_DIR "libexec")
  endif()

  get_filename_component(
    HOST_PREBUILT_ROOT
    "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/${ANDROID_HOST_TAG}"
    ABSOLUTE)

  set(QT_EXECUTABLE ${HOST_PREBUILT_ROOT}/${QT_EXE_DIR}/${target}${EXE_SUFFIX})
  if(NOT EXISTS ${QT_EXECUTABLE})
    message(
      FATAL_ERROR
        "Unable to add Qt${QT_VERSION_MAJOR}::${target} due to missing ${QT_EXECUTABLE}"
    )
  endif()
  add_executable(Qt${QT_VERSION_MAJOR}::${target} IMPORTED GLOBAL)
  set_property(TARGET Qt${QT_VERSION_MAJOR}::${target}
               PROPERTY IMPORTED_LOCATION ${QT_EXECUTABLE})
endfunction()

add_qt_target(uic)
add_qt_target(moc)
add_qt_target(rcc)

# Registers a Qt5 link target..
function(add_qt_shared_lib target link target_deps)
  if(NOT TARGET Qt${QT_VERSION_MAJOR}::${target})
    add_library(Qt${QT_VERSION_MAJOR}::${target} INTERFACE IMPORTED GLOBAL)
    if(QT5_LINK_PATH)
      set(link "${link} ${QT5_LINK_PATH}")
    endif()
    set_target_properties(
      Qt${QT_VERSION_MAJOR}::${target}
      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${QT5_INCLUDE_DIRS}"
                 INTERFACE_LINK_LIBRARIES "${link}"
                 INTERFACE_COMPILE_DEFINITIONS "QT5_STATICLIB")

    android_license(
      TARGET Qt${QT_VERSION_MAJOR}::${target}
      LIBNAME "Qt ${target}"
      URL "https://android.googlesource.com/platform/external/qt5/+/refs/heads/upstream-6.5.3"
      SPDX "LGPL-3.0-only"
      LICENSE
        "https://doc.qt.io/qt-6.5/qt${target}-index.html#licenses-and-attributions"
      LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.LGPLv3")
    # Update the dependencies
    foreach(dep ${target_deps})
      target_link_libraries(Qt${QT_VERSION_MAJOR}::${target} INTERFACE ${dep})
    endforeach()
  endif()

endfunction()

set(QT5_INCLUDE_DIRS
    ${PREBUILT_ROOT}/include
    ${PREBUILT_ROOT}/include/QtCore
    ${PREBUILT_ROOT}/include/QtCore/6.5.3
    ${PREBUILT_ROOT}/include/QtCore/6.5.3/QtCore
    ${PREBUILT_ROOT}/include/QtGui
    ${PREBUILT_ROOT}/include/QtGui/6.5.3
    ${PREBUILT_ROOT}/include/QtSvg
    ${PREBUILT_ROOT}/include/QtWidgets)
if(QTWEBENGINE)
    list(
      APPEND
      QT5_INCLUDE_DIRS
      ${PREBUILT_ROOT}/include/QtWebChannel
      ${PREBUILT_ROOT}/include/QtWebEngineWidgets
      ${PREBUILT_ROOT}/include/QtWebSockets
    )
endif()
set(QT5_INCLUDE_DIR ${QT5_INCLUDE_DIRS})
set(QT5_LINK_PATH "-L${PREBUILT_ROOT}/lib")
set(QT5_DEFINITIONS "-DQT5_STATICLIB")
set(QT5_FOUND TRUE)

if(DARWIN_X86_64 OR DARWIN_AARCH64)
  set(QT5_LIBRARIES -L${PREBUILT_ROOT}/lib ${QT5_LIBRARIES})
  set(QT5_SHARED_DEPENDENCIES
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}Core${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_VERSION_MAJOR}Core${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}Widgets${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_VERSION_MAJOR}Widgets${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}Gui${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_VERSION_MAJOR}Gui${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}Svg${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_VERSION_MAJOR}Svg${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}Network${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_VERSION_MAJOR}Network${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}DBus${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_VERSION_MAJOR}DBus${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib
  )

  set(QT5_SHARED_DEPENDENCIES
      ${QT5_SHARED_DEPENDENCIES};
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}SvgWidgets${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_VERSION_MAJOR}SvgWidgets${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/plugins/platforms/libqcocoa${QT_LIBINFIX}.dylib>lib64/qt/plugins/platforms/libqcocoa${QT_LIBINFIX}.dylib;
      ${PREBUILT_ROOT}/plugins/styles/libqmacstyle${QT_LIBINFIX}.dylib>lib64/qt/plugins/styles/libqmacstyle${QT_LIBINFIX}.dylib;
      ${PREBUILT_ROOT}/plugins/iconengines/libqsvgicon${QT_LIBINFIX}.dylib>lib64/qt/plugins/iconengines/libqsvgicon${QT_LIBINFIX}.dylib;
      ${PREBUILT_ROOT}/plugins/imageformats/libqgif${QT_LIBINFIX}.dylib>lib64/qt/plugins/imageformats/libqgif${QT_LIBINFIX}.dylib;
      ${PREBUILT_ROOT}/plugins/imageformats/libqicns${QT_LIBINFIX}.dylib>lib64/qt/plugins/imageformats/libqicns${QT_LIBINFIX}.dylib;
      ${PREBUILT_ROOT}/plugins/imageformats/libqico${QT_LIBINFIX}.dylib>lib64/qt/plugins/imageformats/libqico${QT_LIBINFIX}.dylib;
      ${PREBUILT_ROOT}/plugins/imageformats/libqjpeg${QT_LIBINFIX}.dylib>lib64/qt/plugins/imageformats/libqjpeg${QT_LIBINFIX}.dylib;
      ${PREBUILT_ROOT}/plugins/imageformats/libqmacheif${QT_LIBINFIX}.dylib>lib64/qt/plugins/imageformats/libqmacheif${QT_LIBINFIX}.dylib;
      ${PREBUILT_ROOT}/plugins/imageformats/libqmacjp2${QT_LIBINFIX}.dylib>lib64/qt/plugins/imageformats/libqmacjp2${QT_LIBINFIX}.dylib;
      ${PREBUILT_ROOT}/plugins/imageformats/libqsvg${QT_LIBINFIX}.dylib>lib64/qt/plugins/imageformats/libqsvg${QT_LIBINFIX}.dylib;
      ${PREBUILT_ROOT}/plugins/imageformats/libqtga${QT_LIBINFIX}.dylib>lib64/qt/plugins/imageformats/libqtga${QT_LIBINFIX}.dylib;
      ${PREBUILT_ROOT}/plugins/imageformats/libqtiff${QT_LIBINFIX}.dylib>lib64/qt/plugins/imageformats/libqtiff${QT_LIBINFIX}.dylib;
      ${PREBUILT_ROOT}/plugins/imageformats/libqwbmp${QT_LIBINFIX}.dylib>lib64/qt/plugins/imageformats/libqwbmp${QT_LIBINFIX}.dylib;
      ${PREBUILT_ROOT}/plugins/imageformats/libqwebp${QT_LIBINFIX}.dylib>lib64/qt/plugins/imageformats/libqwebp${QT_LIBINFIX}.dylib
  )

  add_qt_shared_lib(Core "-lQt${QT_VERSION_MAJOR}Core${QT_LIBINFIX}" "")
  add_qt_shared_lib(Gui "-lQt${QT_VERSION_MAJOR}Gui${QT_LIBINFIX}"
                    "Qt${QT_VERSION_MAJOR}::Core")
  add_qt_shared_lib(Widgets "-lQt${QT_VERSION_MAJOR}Widgets${QT_LIBINFIX}"
                    "Qt${QT_VERSION_MAJOR}::Gui")
  add_qt_shared_lib(Svg "-lQt${QT_VERSION_MAJOR}Svg${QT_LIBINFIX}"
                    "Qt${QT_VERSION_MAJOR}::Widgets")
  add_qt_shared_lib(SvgWidgets "-lQt${QT_VERSION_MAJOR}SvgWidgets${QT_LIBINFIX}"
                    "Qt${QT_VERSION_MAJOR}::Widgets")

  if(QTWEBENGINE)
    add_qt_shared_lib(Network "-lQt${QT_VERSION_MAJOR}Network${QT_LIBINFIX}"
                      "Qt${QT_VERSION_MAJOR}::Core")

    list(
      APPEND
      QT5_WEBENGINE_SHARED_DEPENDENCIES
      ${PREBUILT_ROOT}/libexec/QtWebEngineProcess>lib64/qt/libexec/QtWebEngineProcess
      ${PREBUILT_ROOT}/translations/qtwebengine_locales/*.pak>>lib64/qt/translations/qtwebengine_locales
      ${PREBUILT_ROOT}/resources/icudtl.dat>lib64/qt/resources/icudtl.dat
      ${PREBUILT_ROOT}/resources/qtwebengine_devtools_resources.pak>lib64/qt/resources/qtwebengine_devtools_resources.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources.pak>lib64/qt/resources/qtwebengine_resources.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources_100p.pak>lib64/qt/resources/qtwebengine_resources_100p.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources_200p.pak>lib64/qt/resources/qtwebengine_resources_200p.pak
      # The copies below cannot be symlinks, as they will break the qtwebengine
      # in linux see b/232960190
      ${PREBUILT_ROOT}/resources/icudtl.dat>lib64/qt/libexec/icudtl.dat
      ${PREBUILT_ROOT}/resources/qtwebengine_devtools_resources.pak>lib64/qt/libexec/qtwebengine_devtools_resources.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources.pak>lib64/qt/libexec/qtwebengine_resources.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources_100p.pak>lib64/qt/libexec/qtwebengine_resources_100p.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources_200p.pak>lib64/qt/libexec/qtwebengine_resources_200p.pak
      # BUG: 143948083 WebEngine expects the locales directory to not be under
      # translations
      ${PREBUILT_ROOT}/translations/qtwebengine_locales/*.pak>>lib64/qt/libexec/qtwebengine_locales
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}WebChannel${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_VERSION_MAJOR}WebChannel${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}WebEngineCore${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_VERSION_MAJOR}WebEngineCore${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}WebEngineWidgets${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_VERSION_MAJOR}WebEngineWidgets${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}WebSockets${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_VERSION_MAJOR}WebSockets${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib
    )
    list(
      APPEND
      QT5_SHARED_DEPENDENCIES
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}Qml${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_VERSION_MAJOR}Qml${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}Quick${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_VERSION_MAJOR}Quick${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}QuickWidgets${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_VERSION_MAJOR}QuickWidgets${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}Positioning${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_VERSION_MAJOR}Positioning${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}QmlModels${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_VERSION_MAJOR}QmlModels${QT_LIBINFIX}.${QT_LIB_VERSION}.dylib;
    )
    # Qt6 build does not contain Qml library
    add_qt_shared_lib(
      WebChannel "-lQt${QT_VERSION_MAJOR}WebChannel${QT_LIBINFIX}"
      "Qt${QT_VERSION_MAJOR}::Core")
    add_qt_shared_lib(
      WebSockets "-lQt${QT_VERSION_MAJOR}WebSockets${QT_LIBINFIX}"
      "Qt${QT_VERSION_MAJOR}::Core")
    add_qt_shared_lib(
      WebEngineWidgets "-lQt${QT_VERSION_MAJOR}WebEngineWidgets${QT_LIBINFIX}"
      "Qt${QT_VERSION_MAJOR}::Core")
    add_qt_shared_lib(
      WebEngineCore "-lQt${QT_VERSION_MAJOR}WebEngineCore${QT_LIBINFIX}"
      "Qt${QT_VERSION_MAJOR}::Core")
  endif()

  # Note: this will only set the property for install targets, not during build.
  set(QT5_SHARED_PROPERTIES
      "INSTALL_RPATH>=@loader_path/lib64/qt/libexec;INSTALL_RPATH>=@loader_path/lib64/qt/lib;INSTALL_RPATH>=@loader_path/lib64/qt/plugins"
  )
  set(QT5_LIBRARIES
      ${QT5_LIBRARIES}
      -lQt${QT_VERSION_MAJOR}Network${QT_LIBINFIX}
      -lQt${QT_VERSION_MAJOR}WebChannel${QT_LIBINFIX}
      -lQt${QT_VERSION_MAJOR}WebEngineWidgets${QT_LIBINFIX}
      -lQt${QT_VERSION_MAJOR}WebSockets)
elseif(WINDOWS_MSVC_X86_64)
  # On Windows, Qt provides us with a qtmain.lib which helps us write a cross-
  # platform main() function (because WinMain() is the entry point to a GUI
  # application on Windows). Exactly how qtmain does this is it defines
  # WinMain() for us, and inside WinMain(), it calls a function that we provide.
  # For mingw, this function is qMain(), and for msvc, it's just main() (see qt-
  # src/.../qtmain_lib.cpp for more info). So, in short, for msvc, qtmain.lib
  # --> WinMain() --> main() --> qt_main() and for mingw, qtmain.a --> WinMain()
  # --> qMain() --> qt_main()
  #
  # Clang/VS doesn't support linking directly to dlls. We linking to the import
  # libraries instead (.lib).
  set(QT5_LIBRARIES ${PREBUILT_ROOT}/lib/qtmain.lib)
  set(QT5_LINK_PATH "")
  # Obtained by running ListDlls.exe from sysinternals tool
  set(QT5_SHARED_DEPENDENCIES
      ${PREBUILT_ROOT}/bin/Qt${QT_VERSION_MAJOR}Svg${QT_LIBINFIX}.dll>lib64/qt/lib/Qt${QT_VERSION_MAJOR}Svg${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/bin/Qt${QT_VERSION_MAJOR}Core${QT_LIBINFIX}.dll>lib64/qt/lib/Qt${QT_VERSION_MAJOR}Core${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/bin/Qt${QT_VERSION_MAJOR}Gui${QT_LIBINFIX}.dll>lib64/qt/lib/Qt${QT_VERSION_MAJOR}Gui${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/bin/Qt${QT_VERSION_MAJOR}Widgets${QT_LIBINFIX}.dll>lib64/qt/lib/Qt${QT_VERSION_MAJOR}Widgets${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/bin/Qt${QT_VERSION_MAJOR}Network${QT_LIBINFIX}.dll>lib64/qt/lib/Qt${QT_VERSION_MAJOR}Network${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/bin/Qt${QT_VERSION_MAJOR}OpenGL${QT_LIBINFIX}.dll>lib64/qt/lib/Qt${QT_VERSION_MAJOR}OpenGL${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/bin/Qt${QT_VERSION_MAJOR}Qml${QT_LIBINFIX}.dll>lib64/qt/lib/Qt${QT_VERSION_MAJOR}Qml${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/bin/Qt${QT_VERSION_MAJOR}Positioning${QT_LIBINFIX}.dll>lib64/qt/lib/Qt${QT_VERSION_MAJOR}Positioning${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/bin/Qt${QT_VERSION_MAJOR}QmlModels${QT_LIBINFIX}.dll>lib64/qt/lib/Qt${QT_VERSION_MAJOR}QmlModels${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/bin/Qt${QT_VERSION_MAJOR}Quick${QT_LIBINFIX}.dll>lib64/qt/lib/Qt${QT_VERSION_MAJOR}Quick${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/bin/Qt${QT_VERSION_MAJOR}QuickWidgets${QT_LIBINFIX}.dll>lib64/qt/lib/Qt${QT_VERSION_MAJOR}QuickWidgets${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/bin/Qt${QT_VERSION_MAJOR}SvgWidgets${QT_LIBINFIX}.dll>lib64/qt/lib/Qt${QT_VERSION_MAJOR}SvgWidgets${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/bin/Qt${QT_VERSION_MAJOR}WebChannel${QT_LIBINFIX}.dll>lib64/qt/lib/Qt${QT_VERSION_MAJOR}WebChannel${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/bin/Qt${QT_VERSION_MAJOR}WebEngineCore${QT_LIBINFIX}.dll>lib64/qt/lib/Qt${QT_VERSION_MAJOR}WebEngineCore${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/bin/Qt${QT_VERSION_MAJOR}WebEngineWidgets${QT_LIBINFIX}.dll>lib64/qt/lib/Qt${QT_VERSION_MAJOR}WebEngineWidgets${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/bin/Qt${QT_VERSION_MAJOR}WebSockets${QT_LIBINFIX}.dll>lib64/qt/lib/Qt${QT_VERSION_MAJOR}WebSockets${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/plugins/platforms/qwindows${QT_LIBINFIX}.dll>lib64/qt/plugins/platforms/qwindows${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/plugins/iconengines/qsvgicon${QT_LIBINFIX}.dll>lib64/qt/plugins/iconengines/qsvgicon${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/plugins/imageformats/qgif${QT_LIBINFIX}.dll>lib64/qt/plugins/imageformats/qgif${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/plugins/imageformats/qicns${QT_LIBINFIX}.dll>lib64/qt/plugins/imageformats/qicns${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/plugins/imageformats/qico${QT_LIBINFIX}.dll>lib64/qt/plugins/imageformats/qico${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/plugins/imageformats/qjpeg${QT_LIBINFIX}.dll>lib64/qt/plugins/imageformats/qjpeg${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/plugins/imageformats/qsvg${QT_LIBINFIX}.dll>lib64/qt/plugins/imageformats/qsvg${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/plugins/imageformats/qtga${QT_LIBINFIX}.dll>lib64/qt/plugins/imageformats/qtga${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/plugins/imageformats/qtiff${QT_LIBINFIX}.dll>lib64/qt/plugins/imageformats/qtiff${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/plugins/imageformats/qwbmp${QT_LIBINFIX}.dll>lib64/qt/plugins/imageformats/qwbmp${QT_LIBINFIX}.dll;
      ${PREBUILT_ROOT}/plugins/imageformats/qwebp${QT_LIBINFIX}.dll>lib64/qt/plugins/imageformats/qwebp${QT_LIBINFIX}.dll;
  )

  add_qt_shared_lib(Core "${PREBUILT_ROOT}/lib/Qt${QT_VERSION_MAJOR}Core${QT_LIBINFIX}.lib" "")
  add_qt_shared_lib(Gui "${PREBUILT_ROOT}/lib/Qt${QT_VERSION_MAJOR}Gui${QT_LIBINFIX}.lib" "Qt${QT_VERSION_MAJOR}::Core")
  add_qt_shared_lib(Widgets "${PREBUILT_ROOT}/lib/Qt${QT_VERSION_MAJOR}Widgets${QT_LIBINFIX}.lib"
                    "Qt${QT_VERSION_MAJOR}::Gui")
  add_qt_shared_lib(Svg "${PREBUILT_ROOT}/lib/Qt${QT_VERSION_MAJOR}Svg${QT_LIBINFIX}.lib"
                    "Qt${QT_VERSION_MAJOR}::Widgets")
  add_qt_shared_lib(SvgWidgets "${PREBUILT_ROOT}/lib/Qt${QT_VERSION_MAJOR}SvgWidgets${QT_LIBINFIX}.lib"
                    "Qt${QT_VERSION_MAJOR}::Widgets")

  add_qt_shared_lib(Network "${PREBUILT_ROOT}/lib/Qt${QT_VERSION_MAJOR}Network${QT_LIBINFIX}.lib"
                    "Qt${QT_VERSION_MAJOR}::Core")
  add_qt_shared_lib(Qml "${PREBUILT_ROOT}/lib/Qt${QT_VERSION_MAJOR}Qml${QT_LIBINFIX}.lib"
                    "Qt${QT_VERSION_MAJOR}::Network")
  add_qt_shared_lib(
    WebEngineCore "${PREBUILT_ROOT}/lib/Qt${QT_VERSION_MAJOR}WebEngineCore${QT_LIBINFIX}.lib"
    "Qt${QT_VERSION_MAJOR}::Network")
  add_qt_shared_lib(
    WebChannel "${PREBUILT_ROOT}/lib/Qt${QT_VERSION_MAJOR}WebChannel${QT_LIBINFIX}.lib"
    "Qt${QT_VERSION_MAJOR}::WebEngineCore;Qt${QT_VERSION_MAJOR}::Qml")
  add_qt_shared_lib(
    WebSockets "${PREBUILT_ROOT}/lib/Qt${QT_VERSION_MAJOR}WebSockets${QT_LIBINFIX}.lib"
    "Qt${QT_VERSION_MAJOR}::WebEngineCore;Qt${QT_VERSION_MAJOR}::Qml")
  add_qt_shared_lib(
    WebEngineWidgets "${PREBUILT_ROOT}/lib/Qt${QT_VERSION_MAJOR}WebEngineWidgets${QT_LIBINFIX}.lib"
    "Qt${QT_VERSION_MAJOR}::WebEngineCore;Qt${QT_VERSION_MAJOR}::Qml")

  list(
    APPEND
    QT5_SHARED_DEPENDENCIES
    ${PREBUILT_ROOT}/bin/QtWebEngineProcess.exe>lib64/qt/bin/QtWebEngineProcess.exe
    ${PREBUILT_ROOT}/translations/qtwebengine_locales/*.pak>>lib64/qt/translations/qtwebengine_locales
    ${PREBUILT_ROOT}/resources/icudtl.dat>lib64/qt/resources/icudtl.dat
    ${PREBUILT_ROOT}/resources/qtwebengine_devtools_resources.pak>lib64/qt/resources/qtwebengine_devtools_resources.pak
    ${PREBUILT_ROOT}/resources/qtwebengine_resources.pak>lib64/qt/resources/qtwebengine_resources.pak
    ${PREBUILT_ROOT}/resources/qtwebengine_resources_100p.pak>lib64/qt/resources/qtwebengine_resources_100p.pak
    ${PREBUILT_ROOT}/resources/qtwebengine_resources_200p.pak>lib64/qt/resources/qtwebengine_resources_200p.pak
    # TODO: These 6 are copies of the files immediately above. It would be nice
    # to make these symbolic links rather than copies.
    ${PREBUILT_ROOT}/resources/icudtl.dat>lib64/qt/bin/icudtl.dat
    ${PREBUILT_ROOT}/resources/qtwebengine_devtools_resources.pak>lib64/qt/bin/qtwebengine_devtools_resources.pak
    ${PREBUILT_ROOT}/resources/qtwebengine_resources.pak>lib64/qt/bin/qtwebengine_resources.pak
    ${PREBUILT_ROOT}/resources/qtwebengine_resources_100p.pak>lib64/qt/bin/qtwebengine_resources_100p.pak
    ${PREBUILT_ROOT}/resources/qtwebengine_resources_200p.pak>lib64/qt/bin/qtwebengine_resources_200p.pak
    # BUG: 143948083 WebEngine expects the locales directory to not be under
    # translations
    ${PREBUILT_ROOT}/translations/qtwebengine_locales/*.pak>>lib64/qt/bin/qtwebengine_locales
  )
elseif(LINUX)
  set(QT5_LIBRARIES -L${PREBUILT_ROOT}/lib ${QT5_LIBRARIES})
  # LD_DEBUG=libs ./emulator @P_64 2>&1 | grep qt | grep init
  set(QT5_SHARED_DEPENDENCIES
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}Core${QT_LIBINFIX}.so.6>lib64/qt/lib/libQt${QT_VERSION_MAJOR}Core${QT_LIBINFIX}.so.6;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}Gui${QT_LIBINFIX}.so.6>lib64/qt/lib/libQt${QT_VERSION_MAJOR}Gui${QT_LIBINFIX}.so.6;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}Widgets${QT_LIBINFIX}.so.6>lib64/qt/lib/libQt${QT_VERSION_MAJOR}Widgets${QT_LIBINFIX}.so.6;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}Svg${QT_LIBINFIX}.so.6>lib64/qt/lib/libQt${QT_VERSION_MAJOR}Svg${QT_LIBINFIX}.so.6;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}SvgWidgets${QT_LIBINFIX}.so.6>lib64/qt/lib/libQt${QT_VERSION_MAJOR}SvgWidgets${QT_LIBINFIX}.so.6;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}DBus${QT_LIBINFIX}.so.6>lib64/qt/lib/libQt${QT_VERSION_MAJOR}DBus${QT_LIBINFIX}.so.6;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}Network${QT_LIBINFIX}.so.6>lib64/qt/lib/libQt${QT_VERSION_MAJOR}Network${QT_LIBINFIX}.so.6;
      ${PREBUILT_ROOT}/plugins/iconengines/libqsvgicon${QT_LIBINFIX}.so>lib64/qt/plugins/iconengines/libqsvgicon${QT_LIBINFIX}.so;
      ${PREBUILT_ROOT}/plugins/imageformats/libqwebp${QT_LIBINFIX}.so>lib64/qt/plugins/imageformats/libqwebp${QT_LIBINFIX}.so;
      ${PREBUILT_ROOT}/plugins/imageformats/libqgif${QT_LIBINFIX}.so>lib64/qt/plugins/imageformats/libqgif${QT_LIBINFIX}.so;
      ${PREBUILT_ROOT}/plugins/imageformats/libqicns${QT_LIBINFIX}.so>lib64/qt/plugins/imageformats/libqicns${QT_LIBINFIX}.so;
      ${PREBUILT_ROOT}/plugins/imageformats/libqico${QT_LIBINFIX}.so>lib64/qt/plugins/imageformats/libqico${QT_LIBINFIX}.so;
      ${PREBUILT_ROOT}/plugins/imageformats/libqjpeg${QT_LIBINFIX}.so>lib64/qt/plugins/imageformats/libqjpeg${QT_LIBINFIX}.so;
      ${PREBUILT_ROOT}/plugins/imageformats/libqsvg${QT_LIBINFIX}.so>lib64/qt/plugins/imageformats/libqsvg${QT_LIBINFIX}.so;
      ${PREBUILT_ROOT}/plugins/imageformats/libqtga${QT_LIBINFIX}.so>lib64/qt/plugins/imageformats/libqtga${QT_LIBINFIX}.so;
      ${PREBUILT_ROOT}/plugins/imageformats/libqtiff${QT_LIBINFIX}.so>lib64/qt/plugins/imageformats/libqtiff${QT_LIBINFIX}.so;
      ${PREBUILT_ROOT}/plugins/imageformats/libqwbmp${QT_LIBINFIX}.so>lib64/qt/plugins/imageformats/libqwbmp${QT_LIBINFIX}.so
  )

  list(APPEND QT5_SHARED_DEPENDENCIES
    ${PREBUILT_ROOT}/plugins/platforms/libqxcb${QT_LIBINFIX}.so>lib64/qt/plugins/platforms/libqxcb${QT_LIBINFIX}.so;
    ${PREBUILT_ROOT}/plugins/platforms/libqlinuxfb${QT_LIBINFIX}.so>lib64/qt/plugins/platforms/libqlinuxfb${QT_LIBINFIX}.so;
    ${PREBUILT_ROOT}/plugins/platforms/libqminimal${QT_LIBINFIX}.so>lib64/qt/plugins/platforms/libqminimal${QT_LIBINFIX}.so;
    ${PREBUILT_ROOT}/plugins/platforms/libqoffscreen${QT_LIBINFIX}.so>lib64/qt/plugins/platforms/libqoffscreen${QT_LIBINFIX}.so;
    ${PREBUILT_ROOT}/plugins/platforms/libqvnc${QT_LIBINFIX}.so>lib64/qt/plugins/platforms/libqvnc${QT_LIBINFIX}.so;
    ${PREBUILT_ROOT}/plugins/platforminputcontexts/libcomposeplatforminputcontextplugin${QT_LIBINFIX}.so>lib64/qt/plugins/platforminputcontexts/libcomposeplatforminputcontextplugin${QT_LIBINFIX}.so;
    ${PREBUILT_ROOT}/plugins/platforminputcontexts/libibusplatforminputcontextplugin${QT_LIBINFIX}.so>lib64/qt/plugins/platforminputcontexts/libibusplatforminputcontextplugin${QT_LIBINFIX}.so;
    ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}XcbQpa${QT_LIBINFIX}.so.6>lib64/qt/lib/libQt${QT_VERSION_MAJOR}XcbQpa${QT_LIBINFIX}.so.6;
    ${PREBUILT_ROOT}/lib/libfreetype.so.6>lib64/qt/lib/libfreetype.so.6;
    ${PREBUILT_ROOT}/lib/libxkbcommon.so.0>lib64/qt/lib/libxkbcommon.so.0;
    ${PREBUILT_ROOT}/lib/libX11-xcb.so.1>lib64/qt/lib/libX11-xcb.so.1;
    ${PREBUILT_ROOT}/lib/libXau.so.6>lib64/qt/lib/libXau.so.6;
    ${PREBUILT_ROOT}/lib/libXdmcp.so.6>lib64/qt/lib/libXdmcp.so.6;
    ${PREBUILT_ROOT}/lib/libxcb-xkb.so.1>lib64/qt/lib/libxcb-xkb.so.1;
    ${PREBUILT_ROOT}/lib/libxcb-cursor.so.0>lib64/qt/lib/libxcb-cursor.so.0;
    ${PREBUILT_ROOT}/lib/libxcb-icccm.so.4>lib64/qt/lib/libxcb-icccm.so.4;
    ${PREBUILT_ROOT}/lib/libxcb-image.so.0>lib64/qt/lib/libxcb-image.so.0;
    ${PREBUILT_ROOT}/lib/libxcb-keysyms.so.1>lib64/qt/lib/libxcb-keysyms.so.1;
    ${PREBUILT_ROOT}/lib/libxcb-randr.so.0>lib64/qt/lib/libxcb-randr.so.0;
    ${PREBUILT_ROOT}/lib/libxcb-render-util.so.0>lib64/qt/lib/libxcb-render-util.so.0;
    ${PREBUILT_ROOT}/lib/libxcb-render.so.0>lib64/qt/lib/libxcb-render.so.0;
    ${PREBUILT_ROOT}/lib/libxcb-shape.so.0>lib64/qt/lib/libxcb-shape.so.0;
    ${PREBUILT_ROOT}/lib/libxcb-shm.so.0>lib64/qt/lib/libxcb-shm.so.0;
    ${PREBUILT_ROOT}/lib/libxcb-sync.so.1>lib64/qt/lib/libxcb-sync.so.1;
    ${PREBUILT_ROOT}/lib/libxcb-util.so.1>lib64/qt/lib/libxcb-util.so.1;
    ${PREBUILT_ROOT}/lib/libxcb-xfixes.so.0>lib64/qt/lib/libxcb-xfixes.so.0;
    ${PREBUILT_ROOT}/lib/libxkbcommon-x11.so.0>lib64/qt/lib/libxkbcommon-x11.so.0;
    ${PREBUILT_ROOT}/lib/libfontconfig.so.1>lib64/qt/lib/libfontconfig.so.1)
  if(QTWEBENGINE)
    list(APPEND QT5_SHARED_DEPENDENCIES
      ${PREBUILT_ROOT}/lib/libpcre2-16.so.0>lib64/qt/lib/libpcre2-16.so.0)
  endif()

  set(QT5_SHARED_PROPERTIES
      "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/qt/lib'  -Wl,--disable-new-dtags;LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/qt/lib/plugins'  -Wl,--disable-new-dtags"
  )
  set(QT5_LIBRARIES ${QT5_LIBRARIES} -lQt${QT_VERSION_MAJOR}Network)

  add_qt_shared_lib(Core "-lQt${QT_VERSION_MAJOR}Core${QT_LIBINFIX}" "")
  add_qt_shared_lib(Gui "-lQt${QT_VERSION_MAJOR}Gui${QT_LIBINFIX}" "Qt${QT_VERSION_MAJOR}::Core")
  add_qt_shared_lib(Widgets "-lQt${QT_VERSION_MAJOR}Widgets${QT_LIBINFIX}" "Qt${QT_VERSION_MAJOR}::Gui")
  add_qt_shared_lib(Svg "-lQt${QT_VERSION_MAJOR}Svg${QT_LIBINFIX}" "Qt${QT_VERSION_MAJOR}::Widgets")
  add_qt_shared_lib(SvgWidgets "-lQt${QT_VERSION_MAJOR}SvgWidgets${QT_LIBINFIX}" "Qt${QT_VERSION_MAJOR}::Widgets")

  if(QTWEBENGINE)
    add_qt_shared_lib(Network "-lQt${QT_VERSION_MAJOR}Network${QT_LIBINFIX}" "Qt${QT_VERSION_MAJOR}::Core")
    add_qt_shared_lib(WebEngineCore "-lQt${QT_VERSION_MAJOR}WebEngineCore${QT_LIBINFIX}" "Qt${QT_VERSION_MAJOR}::Network")
    add_qt_shared_lib(WebChannel "-lQt${QT_VERSION_MAJOR}WebChannel${QT_LIBINFIX}" "Qt${QT_VERSION_MAJOR}::Network")
    add_qt_shared_lib(WebSockets "-lQt${QT_VERSION_MAJOR}WebSockets${QT_LIBINFIX}" "Qt${QT_VERSION_MAJOR}::Network")
    add_qt_shared_lib(WebEngineWidgets "-lQt${QT_VERSION_MAJOR}WebEngineWidgets${QT_LIBINFIX}"
                      "Qt${QT_VERSION_MAJOR}::Network")

    list(APPEND QT5_LIBRARIES ${QT5_LIBRARIES} -lQt${QT_VERSION_MAJOR}WebChannel${QT_LIBINFIX}
         -lQt${QT_VERSION_MAJOR}WebEngineWidgets${QT_LIBINFIX} -lQt${QT_VERSION_MAJOR}WebSockets${QT_LIBINFIX})
    list(
      APPEND
      QT5_WEBENGINE_SHARED_DEPENDENCIES
      ${PREBUILT_ROOT}/libexec/QtWebEngineProcess>lib64/qt/libexec/QtWebEngineProcess
      ${PREBUILT_ROOT}/translations/qtwebengine_locales/*.pak>>lib64/qt/translations/qtwebengine_locales
      ${PREBUILT_ROOT}/resources/icudtl.dat>lib64/qt/resources/icudtl.dat
      ${PREBUILT_ROOT}/resources/qtwebengine_devtools_resources.pak>lib64/qt/resources/qtwebengine_devtools_resources.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources.pak>lib64/qt/resources/qtwebengine_resources.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources_100p.pak>lib64/qt/resources/qtwebengine_resources_100p.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources_200p.pak>lib64/qt/resources/qtwebengine_resources_200p.pak
      # The copies below cannot be symlinks, as they will break the qtwebengine
      # in linux see b/232960190
      ${PREBUILT_ROOT}/resources/icudtl.dat>lib64/qt/libexec/icudtl.dat
      ${PREBUILT_ROOT}/resources/qtwebengine_devtools_resources.pak>lib64/qt/libexec/qtwebengine_devtools_resources.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources.pak>lib64/qt/libexec/qtwebengine_resources.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources_100p.pak>lib64/qt/libexec/qtwebengine_resources_100p.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources_200p.pak>lib64/qt/libexec/qtwebengine_resources_200p.pak
      # BUG: 143948083 WebEngine expects the locales directory to not be under
      # translations
      ${PREBUILT_ROOT}/translations/qtwebengine_locales/*.pak>>lib64/qt/libexec/qtwebengine_locales
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}Positioning${QT_LIBINFIX}.so.6>lib64/qt/lib/libQt${QT_VERSION_MAJOR}Positioning${QT_LIBINFIX}.so.6;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}Qml${QT_LIBINFIX}.so.6>lib64/qt/lib/libQt${QT_VERSION_MAJOR}Qml${QT_LIBINFIX}.so.6;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}QmlModels${QT_LIBINFIX}.so.6>lib64/qt/lib/libQt${QT_VERSION_MAJOR}QmlModels${QT_LIBINFIX}.so.6;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}Quick${QT_LIBINFIX}.so.6>lib64/qt/lib/libQt${QT_VERSION_MAJOR}Quick${QT_LIBINFIX}.so.6;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}QuickWidgets${QT_LIBINFIX}.so.6>lib64/qt/lib/libQt${QT_VERSION_MAJOR}QuickWidgets${QT_LIBINFIX}.so.6;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}WebChannel${QT_LIBINFIX}.so.6>lib64/qt/lib/libQt${QT_VERSION_MAJOR}WebChannel${QT_LIBINFIX}.so.6;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}WebEngineCore${QT_LIBINFIX}.so.6>lib64/qt/lib/libQt${QT_VERSION_MAJOR}WebEngineCore${QT_LIBINFIX}.so.6;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}WebEngineWidgets${QT_LIBINFIX}.so.6>lib64/qt/lib/libQt${QT_VERSION_MAJOR}WebEngineWidgets${QT_LIBINFIX}.so.6;
      ${PREBUILT_ROOT}/lib/libQt${QT_VERSION_MAJOR}WebSockets${QT_LIBINFIX}.so.6>lib64/qt/lib/libQt${QT_VERSION_MAJOR}WebSockets${QT_LIBINFIX}.so.6;
      ${PREBUILT_ROOT}/lib/libjpeg.so.8>lib64/qt/lib/libjpeg.so.8
    )

    list(
      APPEND
      QT5_SHARED_PROPERTIES
      "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/qt/libexec'  -Wl,--disable-new-dtags"
    )
  endif()

endif()

set(QT5_LIBRARIES ${QT5_LIBRARIES} -lQt${QT_VERSION_MAJOR}Widgets${QT_LIBINFIX} -lQt${QT_VERSION_MAJOR}Gui${QT_LIBINFIX}
                  -lQt${QT_VERSION_MAJOR}Core${QT_LIBINFIX} -lQt${QT_VERSION_MAJOR}Svg${QT_LIBINFIX} -lQt${QT_VERSION_MAJOR}SvgWidgets${QT_LIBINFIX})

set(PACKAGE_EXPORT
    QT5_INCLUDE_DIR
    QT5_INCLUDE_DIRS
    QT5_LIBRARIES
    QT5_FOUND
    QT5_DEFINITIONS
    CMAKE_AUTOMOC
    CMAKE_AUTOUIC
    CMAKE_AUTORCC
    CMAKE_AUTORCC_OPTIONS
    QT_MOC_EXECUTABLE
    QT_UIC_EXECUTABLE
    QT_RCC_EXECUTABLE
    QT_VERSION_MAJOR
    QT5_SHARED_DEPENDENCIES
    QT5_WEBENGINE_SHARED_DEPENDENCIES
    QT5_LINK_PATH
    QT5_SHARED_PROPERTIES
    QT_VERSION_MINOR)

android_license(
  TARGET QT5_SHARED_DEPENDENCIES
  LIBNAME "Qt 6"
  URL "https://android.googlesource.com/platform/external/qt5/+/refs/heads/upstream-6.5.3"
  SPDX "LGPL-3.0-only"
  LICENSE "https://doc.qt.io/qt-6.5/licensing.html"
  LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.LGPLv3")

android_license(
  TARGET QT5_WEBENGINE_SHARED_DEPENDENCIES
  LIBNAME "Qt 6"
  URL "https://android.googlesource.com/platform/external/qt5/+/refs/heads/upstream-6.5.3"
  SPDX "LGPL-3.0-only"
  LICENSE "https://doc.qt.io/qt-6.5/licensing.html"
  LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.LGPLv3")

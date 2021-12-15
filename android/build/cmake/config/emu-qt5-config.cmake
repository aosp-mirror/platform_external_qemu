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

# TODO(joshuaduong): Only mac-aarch64 is using Qt6 at the moment. Remove once all platforms use
# the same version of Qt.
if(QTWEBENGINE AND (ANDROID_TARGET_TAG MATCHES "darwin-aarch64"))
  set(QT_VERSION_MAJOR 6)
  set(QT_VERSION_MINOR 2)
else()
  set(QT_VERSION_MAJOR 5)
  set(QT_VERSION_MINOR 12)
endif()

# TODO: Remove this once we have -no-window working with absolutely no Qt
# dependencies
if(NOT QTWEBENGINE AND ((ANDROID_TARGET_TAG MATCHES "linux-x86_64") OR (ANDROID_TARGET_TAG MATCHES "darwin-x86_64") OR (ANDROID_TARGET_TAG MATCHES "darwin-aarch64")))
  get_filename_component(
    PREBUILT_ROOT
    "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/${ANDROID_TARGET_TAG}-nowebengine"
    ABSOLUTE)

  # Use the host compatible moc, rcc and uic
  get_filename_component(
    HOST_PREBUILT_ROOT
    "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/${ANDROID_HOST_TAG}-nowebengine"
    ABSOLUTE)
else()
  get_filename_component(
    PREBUILT_ROOT
    "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/${ANDROID_TARGET_TAG}"
    ABSOLUTE)

  # Use the host compatible moc, rcc and uic
  if (QTWEBENGINE AND (ANDROID_HOST_TAG MATCHES "darwin-x86_64") AND (ANDROID_TARGET_TAG MATCHES "darwin-aarch64"))
    # We have Qt6 compiler tools specifically for cross-compiling from darwin-x86_64 to
    # darwin-aarch64 (with QtWebEngine)
    get_filename_component(
      HOST_PREBUILT_ROOT
      "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/${ANDROID_HOST_TAG}/qt6"
      ABSOLUTE)
  else()
    get_filename_component(
      HOST_PREBUILT_ROOT
      "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/${ANDROID_HOST_TAG}"
      ABSOLUTE)
  endif()
endif()

set(CMAKE_AUTOMOC TRUE)
set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTORCC TRUE)

# Registers the uic/rcc/moc targets if needed..
function(add_qt_target target)
  if(MSVC)
    set(EXE_SUFFIX ".exe")
  endif()
  if(QT_VERSION_MAJOR EQUAL 6)
    set(QT_EXE_DIR "libexec")
  else()
    set(QT_EXE_DIR "bin")
  endif()

  if(NOT TARGET Qt${QT_VERSION_MAJOR}::${target})
    set(QT_EXECUTABLE ${HOST_PREBUILT_ROOT}/${QT_EXE_DIR}/${target}${EXE_SUFFIX})
    if(NOT EXISTS ${QT_EXECUTABLE})
      message(
        FATAL_ERROR
          "Unable to add Qt${QT_VERSION_MAJOR}::${target} due to missing ${QT_EXECUTABLE}")
    endif()
    add_executable(Qt${QT_VERSION_MAJOR}::${target} IMPORTED GLOBAL)
    set_property(TARGET Qt${QT_VERSION_MAJOR}::${target} PROPERTY IMPORTED_LOCATION
                                                ${QT_EXECUTABLE})
  endif()
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
      URL "https://android.googlesource.com/platform/prebuilts/android-emulator-build/archive/+/refs/heads/emu-master-dev/qt-everywhere-src-5.12.1.tar.xz"
      SPDX "LGPL-3.0-only"
      LICENSE
        "https://doc.qt.io/qt-5.12/qt${target}-index.html#licenses-and-attributions"
      LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.LGPLv3")
    # Update the dependencies
    foreach(dep ${target_deps})
      target_link_libraries(Qt${QT_VERSION_MAJOR}::${target} INTERFACE ${dep})
    endforeach()
  endif()

endfunction()

set(QT5_INCLUDE_DIRS
    ${PREBUILT_ROOT}/include
    ${PREBUILT_ROOT}/include.system
    ${PREBUILT_ROOT}/include.system/QtCore
    ${PREBUILT_ROOT}/include/QtCore
    ${PREBUILT_ROOT}/include/QtGui
    ${PREBUILT_ROOT}/include/QtSvg
    ${PREBUILT_ROOT}/include/QtWidgets
    ${PREBUILT_ROOT}/include/QtWebChannel
    ${PREBUILT_ROOT}/include/QtWebEngineWidgets
    ${PREBUILT_ROOT}/include/QtWebSockets)
set(QT5_INCLUDE_DIR ${QT5_INCLUDE_DIRS})
set(QT5_LINK_PATH "-L${PREBUILT_ROOT}/lib")
set(QT5_DEFINITIONS "-DQT5_STATICLIB")
set(QT5_FOUND TRUE)

if(DARWIN_X86_64 OR DARWIN_AARCH64)
  if (DARWIN_AARCH64 AND QTWEBENGINE)
    set(QT_MAJOR_VERSION 6)
    set(QT_LIB_VERSION 6.2.1)
  else()
    set(QT_MAJOR_VERSION 5)
    set(QT_LIB_VERSION 5.12.1)
  endif()
  set(QT5_LIBRARIES -L${PREBUILT_ROOT}/lib ${QT5_LIBRARIES})
  set(QT5_SHARED_DEPENDENCIES
      ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}CoreAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}CoreAndroidEmu.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}WidgetsAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}WidgetsAndroidEmu.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}GuiAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}GuiAndroidEmu.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}SvgAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}SvgAndroidEmu.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}PrintSupportAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}PrintSupportAndroidEmu.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}NetworkAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}NetworkAndroidEmu.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}DBusAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}DBusAndroidEmu.${QT_LIB_VERSION}.dylib
  )

  if (QT_MAJOR_VERSION EQUAL 5)
    set(QT5_SHARED_DEPENDENCIES
        ${QT5_SHARED_DEPENDENCIES};
        ${PREBUILT_ROOT}/plugins/bearer/libqgenericbearer.dylib>lib64/qt/plugins/bearer/libqgenericbearer.dylib;
        ${PREBUILT_ROOT}/plugins/platforms/libqcocoa.dylib>lib64/qt/plugins/platforms/libqcocoa.dylib;
        ${PREBUILT_ROOT}/plugins/styles/libqmacstyle.dylib>lib64/qt/plugins/styles/libqmacstyle.dylib;
        ${PREBUILT_ROOT}/plugins/iconengines/libqsvgicon.dylib>lib64/qt/plugins/iconengines/libqsvgicon.dylib;
        ${PREBUILT_ROOT}/plugins/imageformats/libqgif.dylib>lib64/qt/plugins/imageformats/libqgif.dylib;
        ${PREBUILT_ROOT}/plugins/imageformats/libqicns.dylib>lib64/qt/plugins/imageformats/libqicns.dylib;
        ${PREBUILT_ROOT}/plugins/imageformats/libqico.dylib>lib64/qt/plugins/imageformats/libqico.dylib;
        ${PREBUILT_ROOT}/plugins/imageformats/libqjpeg.dylib>lib64/qt/plugins/imageformats/libqjpeg.dylib;
        ${PREBUILT_ROOT}/plugins/imageformats/libqmacheif.dylib>lib64/qt/plugins/imageformats/libqmacheif.dylib;
        ${PREBUILT_ROOT}/plugins/imageformats/libqmacjp2.dylib>lib64/qt/plugins/imageformats/libqmacjp2.dylib;
        ${PREBUILT_ROOT}/plugins/imageformats/libqsvg.dylib>lib64/qt/plugins/imageformats/libqsvg.dylib;
        ${PREBUILT_ROOT}/plugins/imageformats/libqtga.dylib>lib64/qt/plugins/imageformats/libqtga.dylib;
        ${PREBUILT_ROOT}/plugins/imageformats/libqtiff.dylib>lib64/qt/plugins/imageformats/libqtiff.dylib;
        ${PREBUILT_ROOT}/plugins/imageformats/libqwbmp.dylib>lib64/qt/plugins/imageformats/libqwbmp.dylib;
        ${PREBUILT_ROOT}/plugins/imageformats/libqwebp.dylib>lib64/qt/plugins/imageformats/libqwebp.dylib
    )
  endif()

  if (QT_MAJOR_VERSION EQUAL 6)
    set(QT_LIBINFIX "AndroidEmu")
    set(QT5_SHARED_DEPENDENCIES
        ${QT5_SHARED_DEPENDENCIES};
        ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}SvgWidgetsAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}SvgWidgetsAndroidEmu.${QT_LIB_VERSION}.dylib;
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
  endif()

  add_qt_shared_lib(Core "-lQt${QT_MAJOR_VERSION}CoreAndroidEmu" "")
  add_qt_shared_lib(Gui "-lQt${QT_MAJOR_VERSION}GuiAndroidEmu" "Qt${QT_MAJOR_VERSION}::Core")
  add_qt_shared_lib(Widgets "-lQt${QT_MAJOR_VERSION}WidgetsAndroidEmu" "Qt${QT_MAJOR_VERSION}::Gui")
  add_qt_shared_lib(Svg "-lQt${QT_MAJOR_VERSION}SvgAndroidEmu" "Qt${QT_MAJOR_VERSION}::Widgets")
  if (QT_MAJOR_VERSION EQUAL 6)
    add_qt_shared_lib(SvgWidgets "-lQt${QT_MAJOR_VERSION}SvgWidgetsAndroidEmu" "Qt${QT_MAJOR_VERSION}::Widgets")
  endif()

  if(QTWEBENGINE)
    add_qt_shared_lib(Network "-lQt${QT_MAJOR_VERSION}NetworkAndroidEmu" "Qt${QT_MAJOR_VERSION}::Core")

    list(
      APPEND
      QT5_SHARED_DEPENDENCIES
      ${PREBUILT_ROOT}/libexec/QtWebEngineProcess>lib64/qt/libexec/QtWebEngineProcess
      ${PREBUILT_ROOT}/translations/qtwebengine_locales/*.pak>>lib64/qt/translations/qtwebengine_locales
      ${PREBUILT_ROOT}/resources/icudtl.dat>lib64/qt/resources/icudtl.dat
      ${PREBUILT_ROOT}/resources/qtwebengine_devtools_resources.pak>lib64/qt/resources/qtwebengine_devtools_resources.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources.pak>lib64/qt/resources/qtwebengine_resources.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources_100p.pak>lib64/qt/resources/qtwebengine_resources_100p.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources_200p.pak>lib64/qt/resources/qtwebengine_resources_200p.pak
      # TODO: These 6 are copies of the files immediately above. It would be
      # nice to make these symbolic links rather than copies.
      ${PREBUILT_ROOT}/resources/icudtl.dat>lib64/qt/libexec/icudtl.dat
      ${PREBUILT_ROOT}/resources/qtwebengine_devtools_resources.pak>lib64/qt/libexec/qtwebengine_devtools_resources.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources.pak>lib64/qt/libexec/qtwebengine_resources.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources_100p.pak>lib64/qt/libexec/qtwebengine_resources_100p.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources_200p.pak>lib64/qt/libexec/qtwebengine_resources_200p.pak
      # BUG: 143948083 WebEngine expects the locales directory to not be under
      # translations
      ${PREBUILT_ROOT}/translations/qtwebengine_locales/*.pak>>lib64/qt/libexec/qtwebengine_locales
      ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}WebChannelAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}WebChannelAndroidEmu.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}WebEngineCoreAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}WebEngineCoreAndroidEmu.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}WebEngineWidgetsAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}WebEngineWidgetsAndroidEmu.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}WebSocketsAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}WebSocketsAndroidEmu.${QT_LIB_VERSION}.dylib
    )
    if(QT_MAJOR_VERSION EQUAL 5)
      add_qt_shared_lib(Qml "-lQt${QT_MAJOR_VERSION}QmlAndroidEmu" "Qt${QT_MAJOR_VERSION}::Network")
      add_qt_shared_lib(WebChannel "-lQt${QT_MAJOR_VERSION}WebChannelAndroidEmu" "Qt${QT_MAJOR_VERSION}::Qml")
      add_qt_shared_lib(WebSockets "-lQt${QT_MAJOR_VERSION}WebSocketsAndroidEmu" "Qt${QT_MAJOR_VERSION}::Qml")
      add_qt_shared_lib(WebEngineWidgets "-lQt${QT_MAJOR_VERSION}WebEngineWidgetsAndroidEmu"
                        "Qt${QT_MAJOR_VERSION}::Qml")
      list(
        APPEND
        QT5_SHARED_DEPENDENCIES
        ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}QmlAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}QmlAndroidEmu.${QT_LIB_VERSION}.dylib;
        ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}QuickAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}QuickAndroidEmu.${QT_LIB_VERSION}.dylib;
        ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}QuickWidgetsAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}QuickWidgetsAndroidEmu.${QT_LIB_VERSION}.dylib;
      )
    elseif(QT_MAJOR_VERSION EQUAL 6)
      list(
        APPEND
        QT5_SHARED_DEPENDENCIES
        ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}QmlAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}QmlAndroidEmu.${QT_LIB_VERSION}.dylib;
        ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}QuickAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}QuickAndroidEmu.${QT_LIB_VERSION}.dylib;
        ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}QuickWidgetsAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}QuickWidgetsAndroidEmu.${QT_LIB_VERSION}.dylib;
        ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}PositioningAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}PositioningAndroidEmu.${QT_LIB_VERSION}.dylib;
        ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}OpenGLAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}OpenGLAndroidEmu.${QT_LIB_VERSION}.dylib;
        ${PREBUILT_ROOT}/lib/libQt${QT_MAJOR_VERSION}QmlModelsAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt${QT_MAJOR_VERSION}QmlModelsAndroidEmu.${QT_LIB_VERSION}.dylib;
      )
      # Qt6 build does not contain Qml library
      add_qt_shared_lib(WebChannel "-lQt${QT_MAJOR_VERSION}WebChannelAndroidEmu" "Qt${QT_MAJOR_VERSION}::Core")
      add_qt_shared_lib(WebSockets "-lQt${QT_MAJOR_VERSION}WebSocketsAndroidEmu" "Qt${QT_MAJOR_VERSION}::Core")
      add_qt_shared_lib(WebEngineWidgets "-lQt${QT_MAJOR_VERSION}WebEngineWidgetsAndroidEmu"
                        "Qt${QT_MAJOR_VERSION}::Core")
      add_qt_shared_lib(WebEngineCore "-lQt${QT_MAJOR_VERSION}WebEngineCoreAndroidEmu"
                        "Qt${QT_MAJOR_VERSION}::Core")
    endif()
  endif()

  # Note: this will only set the property for install targets, not during build.
  set(QT5_SHARED_PROPERTIES
      "INSTALL_RPATH>=@loader_path/lib64/qt/libexec;INSTALL_RPATH>=@loader_path/lib64/qt/lib;INSTALL_RPATH>=@loader_path/lib64/qt/plugins"
  )
  set(QT5_LIBRARIES
      ${QT5_LIBRARIES} -lQt${QT_MAJOR_VERSION}NetworkAndroidEmu -lQt${QT_MAJOR_VERSION}WebChannelAndroidEmu
      -lQt${QT_MAJOR_VERSION}WebEngineWidgetsAndroidEmu -lQt${QT_MAJOR_VERSION}WebSockets)
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
      ${PREBUILT_ROOT}/bin/Qt5SvgAndroidEmu.dll>lib64/qt/lib/Qt5SvgAndroidEmu.dll;
      ${PREBUILT_ROOT}/bin/Qt5CoreAndroidEmu.dll>lib64/qt/lib/Qt5CoreAndroidEmu.dll;
      ${PREBUILT_ROOT}/bin/Qt5GuiAndroidEmu.dll>lib64/qt/lib/Qt5GuiAndroidEmu.dll;
      ${PREBUILT_ROOT}/bin/Qt5WidgetsAndroidEmu.dll>lib64/qt/lib/Qt5WidgetsAndroidEmu.dll;
      ${PREBUILT_ROOT}/bin/Qt5NetworkAndroidEmu.dll>lib64/qt/lib/Qt5NetworkAndroidEmu.dll;
      ${PREBUILT_ROOT}/bin/Qt5PrintSupportAndroidEmu.dll>lib64/qt/lib/Qt5PrintSupportAndroidEmu.dll;
      ${PREBUILT_ROOT}/bin/Qt5QmlAndroidEmu.dll>lib64/qt/lib/Qt5QmlAndroidEmu.dll;
      ${PREBUILT_ROOT}/bin/Qt5PositioningAndroidEmu.dll>lib64/qt/lib/Qt5PositioningAndroidEmu.dll;
      ${PREBUILT_ROOT}/bin/Qt5QuickAndroidEmu.dll>lib64/qt/lib/Qt5QuickAndroidEmu.dll;
      ${PREBUILT_ROOT}/bin/Qt5QuickWidgetsAndroidEmu.dll>lib64/qt/lib/Qt5QuickWidgetsAndroidEmu.dll;
      ${PREBUILT_ROOT}/bin/Qt5WebChannelAndroidEmu.dll>lib64/qt/lib/Qt5WebChannelAndroidEmu.dll;
      ${PREBUILT_ROOT}/bin/Qt5WebEngineCoreAndroidEmu.dll>lib64/qt/lib/Qt5WebEngineCoreAndroidEmu.dll;
      ${PREBUILT_ROOT}/bin/Qt5WebEngineWidgetsAndroidEmu.dll>lib64/qt/lib/Qt5WebEngineWidgetsAndroidEmu.dll;
      ${PREBUILT_ROOT}/bin/Qt5WebSocketsAndroidEmu.dll>lib64/qt/lib/Qt5WebSocketsAndroidEmu.dll;
      ${PREBUILT_ROOT}/plugins/bearer/qgenericbearer.dll>lib64/qt/plugins/bearer/qgenericbearer.dll;
      ${PREBUILT_ROOT}/plugins/platforms/qwindows.dll>lib64/qt/plugins/platforms/qwindows.dll;
      ${PREBUILT_ROOT}/plugins/iconengines/qsvgicon.dll>lib64/qt/plugins/iconengines/qsvgicon.dll;
      ${PREBUILT_ROOT}/plugins/imageformats/qgif.dll>lib64/qt/plugins/imageformats/qgif.dll;
      ${PREBUILT_ROOT}/plugins/imageformats/qicns.dll>lib64/qt/plugins/imageformats/qicns.dll;
      ${PREBUILT_ROOT}/plugins/imageformats/qico.dll>lib64/qt/plugins/imageformats/qico.dll;
      ${PREBUILT_ROOT}/plugins/imageformats/qjpeg.dll>lib64/qt/plugins/imageformats/qjpeg.dll;
      ${PREBUILT_ROOT}/plugins/imageformats/qsvg.dll>lib64/qt/plugins/imageformats/qsvg.dll;
      ${PREBUILT_ROOT}/plugins/imageformats/qtga.dll>lib64/qt/plugins/imageformats/qtga.dll;
      ${PREBUILT_ROOT}/plugins/imageformats/qtiff.dll>lib64/qt/plugins/imageformats/qtiff.dll;
      ${PREBUILT_ROOT}/plugins/imageformats/qwbmp.dll>lib64/qt/plugins/imageformats/qwbmp.dll;
      ${PREBUILT_ROOT}/plugins/imageformats/qwebp.dll>lib64/qt/plugins/imageformats/qwebp.dll;
  )

  add_qt_shared_lib(Core "${PREBUILT_ROOT}/lib/Qt5CoreAndroidEmu.lib" "")
  add_qt_shared_lib(Gui "${PREBUILT_ROOT}/lib/Qt5GuiAndroidEmu.lib" "Qt5::Core")
  add_qt_shared_lib(Widgets "${PREBUILT_ROOT}/lib/Qt5WidgetsAndroidEmu.lib"
                    "Qt5::Gui")
  add_qt_shared_lib(Svg "${PREBUILT_ROOT}/lib/Qt5SvgAndroidEmu.lib"
                    "Qt5::Widgets")

  add_qt_shared_lib(Network "${PREBUILT_ROOT}/lib/Qt5NetworkAndroidEmu.lib"
                    "Qt5::Core")
  add_qt_shared_lib(Qml "${PREBUILT_ROOT}/lib/Qt5QmlAndroidEmu.lib"
                    "Qt5::Network")
  add_qt_shared_lib(
    WebEngineCore "${PREBUILT_ROOT}/lib/Qt5WebEngineCoreAndroidEmu.lib"
    "Qt5::Network")
  add_qt_shared_lib(
    WebChannel "${PREBUILT_ROOT}/lib/Qt5WebChannelAndroidEmu.lib"
    "Qt5::WebEngineCore;Qt5::Qml")
  add_qt_shared_lib(
    WebSockets "${PREBUILT_ROOT}/lib/Qt5WebSocketsAndroidEmu.lib"
    "Qt5::WebEngineCore;Qt5::Qml")
  add_qt_shared_lib(
    WebEngineWidgets "${PREBUILT_ROOT}/lib/Qt5WebEngineWidgetsAndroidEmu.lib"
    "Qt5::WebEngineCore;Qt5::Qml")

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
      ${PREBUILT_ROOT}/lib/libQt5CoreAndroidEmu.so.5>lib64/qt/lib/libQt5CoreAndroidEmu.so.5;
      ${PREBUILT_ROOT}/lib/libQt5GuiAndroidEmu.so.5>lib64/qt/lib/libQt5GuiAndroidEmu.so.5;
      ${PREBUILT_ROOT}/lib/libQt5WidgetsAndroidEmu.so.5>lib64/qt/lib/libQt5WidgetsAndroidEmu.so.5;
      ${PREBUILT_ROOT}/lib/libQt5SvgAndroidEmu.so.5>lib64/qt/lib/libQt5SvgAndroidEmu.so.5;
      ${PREBUILT_ROOT}/lib/libQt5PrintSupportAndroidEmu.so.5>lib64/qt/lib/libQt5PrintSupportAndroidEmu.so.5;
      ${PREBUILT_ROOT}/lib/libQt5DBusAndroidEmu.so.5>lib64/qt/lib/libQt5DBusAndroidEmu.so.5;
      ${PREBUILT_ROOT}/lib/libQt5NetworkAndroidEmu.so.5>lib64/qt/lib/libQt5NetworkAndroidEmu.so.5;
      ${PREBUILT_ROOT}/lib/libQt5XcbQpaAndroidEmu.so.5>lib64/qt/lib/libQt5XcbQpaAndroidEmu.so.5;
      ${PREBUILT_ROOT}/plugins/bearer/libqgenericbearer.so>lib64/qt/plugins/bearer/libqgenericbearer.so;
      ${PREBUILT_ROOT}/plugins/bearer/libqconnmanbearer.so>lib64/qt/plugins/bearer/libqconnmanbearer.so;
      ${PREBUILT_ROOT}/plugins/bearer/libqnmbearer.so>lib64/qt/plugins/bearer/libqnmbearer.so;
      ${PREBUILT_ROOT}/plugins/platforms/libqxcb.so>lib64/qt/plugins/platforms/libqxcb.so;
      ${PREBUILT_ROOT}/plugins/platforminputcontexts/libcomposeplatforminputcontextplugin.so>lib64/qt/plugins/platforminputcontexts/libcomposeplatforminputcontextplugin.so;
      ${PREBUILT_ROOT}/plugins/iconengines/libqsvgicon.so>lib64/qt/plugins/iconengines/libqsvgicon.so;
      ${PREBUILT_ROOT}/plugins/imageformats/libqwebp.so>lib64/qt/plugins/imageformats/libqwebp.so;
      ${PREBUILT_ROOT}/plugins/imageformats/libqgif.so>lib64/qt/plugins/imageformats/libqgif.so;
      ${PREBUILT_ROOT}/plugins/imageformats/libqicns.so>lib64/qt/plugins/imageformats/libqicns.so;
      ${PREBUILT_ROOT}/plugins/imageformats/libqico.so>lib64/qt/plugins/imageformats/libqico.so;
      ${PREBUILT_ROOT}/plugins/imageformats/libqjpeg.so>lib64/qt/plugins/imageformats/libqjpeg.so;
      ${PREBUILT_ROOT}/plugins/imageformats/libqsvg.so>lib64/qt/plugins/imageformats/libqsvg.so;
      ${PREBUILT_ROOT}/plugins/imageformats/libqtga.so>lib64/qt/plugins/imageformats/libqtga.so;
      ${PREBUILT_ROOT}/plugins/imageformats/libqtiff.so>lib64/qt/plugins/imageformats/libqtiff.so;
      ${PREBUILT_ROOT}/plugins/imageformats/libqwbmp.so>lib64/qt/plugins/imageformats/libqwbmp.so
  )
  set(QT5_SHARED_PROPERTIES
      "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/qt/lib';LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/qt/lib/plugins'"
  )
  set(QT5_LIBRARIES ${QT5_LIBRARIES} -lQt5Network)

  add_qt_shared_lib(Core "-lQt5CoreAndroidEmu" "")
  add_qt_shared_lib(Gui "-lQt5GuiAndroidEmu" "Qt5::Core")
  add_qt_shared_lib(Widgets "-lQt5WidgetsAndroidEmu" "Qt5::Gui")
  add_qt_shared_lib(Svg "-lQt5SvgAndroidEmu" "Qt5::Widgets")

  get_filename_component(
    PREBUILT_WEBENGINE_DEPS_ROOT
    "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/common/qtwebengine-deps/${ANDROID_TARGET_TAG}"
    ABSOLUTE)

  if(QTWEBENGINE)
    add_qt_shared_lib(Network "-lQt5NetworkAndroidEmu" "Qt5::Core")
    add_qt_shared_lib(Qml "-lQt5QmlAndroidEmu" "Qt5::Network")
    add_qt_shared_lib(WebChannel "-lQt5WebChannelAndroidEmu" "Qt5::Qml")
    add_qt_shared_lib(WebSockets "-lQt5WebSocketsAndroidEmu" "Qt5::Qml")
    add_qt_shared_lib(WebEngineWidgets "-lQt5WebEngineWidgetsAndroidEmu"
                      "Qt5::Qml")

    list(APPEND QT5_LIBRARIES ${QT5_LIBRARIES} -lQt5WebChannelAndroidEmu
         -lQt5WebEngineWidgetsAndroidEmu -lQt5WebSocketsAndroidEmu)
    list(
      APPEND
      QT5_SHARED_DEPENDENCIES
      ${PREBUILT_ROOT}/libexec/QtWebEngineProcess>lib64/qt/libexec/QtWebEngineProcess
      ${PREBUILT_ROOT}/translations/qtwebengine_locales/*.pak>>lib64/qt/translations/qtwebengine_locales
      ${PREBUILT_ROOT}/resources/icudtl.dat>lib64/qt/resources/icudtl.dat
      ${PREBUILT_ROOT}/resources/qtwebengine_devtools_resources.pak>lib64/qt/resources/qtwebengine_devtools_resources.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources.pak>lib64/qt/resources/qtwebengine_resources.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources_100p.pak>lib64/qt/resources/qtwebengine_resources_100p.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources_200p.pak>lib64/qt/resources/qtwebengine_resources_200p.pak
      # TODO: These six are copies of the files immediately above. It would be
      # nice to make these symbolic links rather than copies.
      ${PREBUILT_ROOT}/resources/icudtl.dat>lib64/qt/libexec/icudtl.dat
      ${PREBUILT_ROOT}/resources/qtwebengine_devtools_resources.pak>lib64/qt/libexec/qtwebengine_devtools_resources.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources.pak>lib64/qt/libexec/qtwebengine_resources.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources_100p.pak>lib64/qt/libexec/qtwebengine_resources_100p.pak
      ${PREBUILT_ROOT}/resources/qtwebengine_resources_200p.pak>lib64/qt/libexec/qtwebengine_resources_200p.pak
      # BUG: 143948083 WebEngine expects the locales directory to not be under
      # translations
      ${PREBUILT_ROOT}/translations/qtwebengine_locales/*.pak>>lib64/qt/libexec/qtwebengine_locales
      ${PREBUILT_ROOT}/lib/libQt5QmlAndroidEmu.so.5>lib64/qt/lib/libQt5QmlAndroidEmu.so.5;
      ${PREBUILT_ROOT}/lib/libQt5QuickAndroidEmu.so.5>lib64/qt/lib/libQt5QuickAndroidEmu.so.5;
      ${PREBUILT_ROOT}/lib/libQt5QuickWidgetsAndroidEmu.so.5>lib64/qt/lib/libQt5QuickWidgetsAndroidEmu.so.5;
      ${PREBUILT_ROOT}/lib/libQt5WebChannelAndroidEmu.so.5>lib64/qt/lib/libQt5WebChannelAndroidEmu.so.5;
      ${PREBUILT_ROOT}/lib/libQt5WebEngineCoreAndroidEmu.so.5>lib64/qt/lib/libQt5WebEngineCoreAndroidEmu.so.5;
      ${PREBUILT_ROOT}/lib/libQt5WebEngineWidgetsAndroidEmu.so.5>lib64/qt/lib/libQt5WebEngineWidgetsAndroidEmu.so.5;
      ${PREBUILT_ROOT}/lib/libQt5WebSocketsAndroidEmu.so.5>lib64/qt/lib/libQt5WebSocketsAndroidEmu.so.5
    )

    list(APPEND QT5_SHARED_PROPERTIES
         "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/qt/libexec'")
  endif()

  if (LINUX_X86_64)
    list(
    APPEND
    QT5_SHARED_DEPENDENCIES
    ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libxkbcommon.so.0.0.0>lib64/qt/lib/libxkbcommon.so;
    ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libxkbcommon.so.0.0.0>lib64/qt/lib/libxkbcommon.so.0;
    ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libxkbcommon.so.0.0.0>lib64/qt/lib/libxkbcommon.so.0.0.0;
    ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libX11-xcb.so.1.0.0>lib64/qt/lib/libX11-xcb.so.1;
    ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libX11-xcb.so.1.0.0>lib64/qt/lib/libX11-xcb.so.1.0;
    ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libX11-xcb.so.1.0.0>lib64/qt/lib/libX11-xcb.so.1.0.0;
    ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libxcb-xkb.so.1.0.0>lib64/qt/lib/libxcb-xkb.so.1;
    ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libxcb-xkb.so.1.0.0>lib64/qt/lib/libxcb-xkb.so.1.0;
    ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libxcb-xkb.so.1.0.0>lib64/qt/lib/libxcb-xkb.so.1.0.0;
    ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libxkbcommon-x11.so.0.0.0>lib64/qt/lib/libxkbcommon-x11.so;
    ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libxkbcommon-x11.so.0.0.0>lib64/qt/lib/libxkbcommon-x11.so.0;
    ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libxkbcommon-x11.so.0.0.0>lib64/qt/lib/libxkbcommon-x11.so.0.0.0;
    ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libsoftokn3.so>lib64/qt/lib/libsoftokn3.so;
    ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libsqlite3.so>lib64/qt/lib/libsqlite3.so;
    ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libfreetype.so.6>lib64/qt/lib/libfreetype.so.6
    ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libfontconfig.so.1.12.0>lib64/qt/lib/libfontconfig.so.1
    )
  endif()

  if (LINUX_AARCH64)
    list(
    APPEND
    QT5_SHARED_DEPENDENCIES
    ${PREBUILT_ROOT}/lib/libfreetype.so.6>lib64/qt/lib/libfreetype.so.6
    )
  endif()

endif()

set(QT5_LIBRARIES ${QT5_LIBRARIES} -lQt5WidgetsAndroidEmu -lQt5GuiAndroidEmu
                  -lQt5CoreAndroidEmu -lQt5SvgAndroidEmu)

set(PACKAGE_EXPORT
    QT5_INCLUDE_DIR
    QT5_INCLUDE_DIRS
    QT5_LIBRARIES
    QT5_FOUND
    QT5_DEFINITIONS
    CMAKE_AUTOMOC
    CMAKE_AUTOUIC
    CMAKE_AUTORCC
    QT_MOC_EXECUTABLE
    QT_UIC_EXECUTABLE
    QT_RCC_EXECUTABLE
    QT_VERSION_MAJOR
    QT5_SHARED_DEPENDENCIES
    QT5_SHARED_PROPERTIES
    QT_VERSION_MINOR)

android_license(
  TARGET QT5_SHARED_DEPENDENCIES
  LIBNAME "Qt 5"
  URL "https://android.googlesource.com/platform/prebuilts/android-emulator-build/archive/+/refs/heads/emu-master-dev/qt-everywhere-src-5.12.1.tar.xz"
  SPDX "LGPL-3.0-only"
  LICENSE "https://doc.qt.io/qt-5/licensing.html"
  LOCAL "${ANDROID_QEMU2_TOP_DIR}/LICENSES/LICENSE.LGPLv3")

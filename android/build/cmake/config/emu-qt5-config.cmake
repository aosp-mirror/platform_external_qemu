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

get_filename_component(PREBUILT_ROOT
                       "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/${ANDROID_TARGET_TAG}"
                       ABSOLUTE)

# Use the host compatible moc, rcc and uic
get_filename_component(HOST_PREBUILT_ROOT
                       "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/${ANDROID_HOST_TAG}"
                       ABSOLUTE)

if(NOT TARGET Qt5::moc)
  set(QT_VERSION_MAJOR 5)
  set(CMAKE_AUTOMOC TRUE)
  set(CMAKE_AUTOUIC TRUE)
  set(CMAKE_AUTORCC TRUE)
  set(QT_MOC_EXECUTABLE ${HOST_PREBUILT_ROOT}/bin/moc)
  set(QT_UIC_EXECUTABLE ${HOST_PREBUILT_ROOT}/bin/uic)
  set(QT_RCC_EXECUTABLE ${HOST_PREBUILT_ROOT}/bin/rcc)
  add_executable(Qt5::moc IMPORTED GLOBAL)
  set_property(TARGET Qt5::moc PROPERTY IMPORTED_LOCATION ${QT_MOC_EXECUTABLE})
  add_executable(Qt5::uic IMPORTED GLOBAL)
  set_property(TARGET Qt5::uic PROPERTY IMPORTED_LOCATION ${QT_UIC_EXECUTABLE})
  add_executable(Qt5::rcc IMPORTED GLOBAL)
  set_property(TARGET Qt5::rcc PROPERTY IMPORTED_LOCATION ${QT_RCC_EXECUTABLE})
endif()

set(QT5_INCLUDE_DIRS
    ${PREBUILT_ROOT}/include
    ${PREBUILT_ROOT}/include.system
    ${PREBUILT_ROOT}/include.system/QtCore
    ${PREBUILT_ROOT}/include/QtCore
    ${PREBUILT_ROOT}/include/QtGui
    ${PREBUILT_ROOT}/include/QtSvg
    ${PREBUILT_ROOT}/include/QtWidgets)

set(QT5_INCLUDE_DIR ${QT5_INCLUDE_DIRS})

set(QT5_DEFINITIONS "-DQT5_STATICLIB")
set(QT5_FOUND TRUE)

if(ANDROID_TARGET_TAG STREQUAL "darwin-x86_64")
  set(QT_LIB_VERSION 5.11.1)
  set(QT5_LIBRARIES -L${PREBUILT_ROOT}/lib)
  set(QT5_SHARED_DEPENDENCIES
      ${PREBUILT_ROOT}/lib/libQt5Core.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt5Core.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt5Widgets.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt5Widgets.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt5Gui.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt5Gui.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt5Svg.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt5Svg.${QT_LIB_VERSION}.dylib;
      ${PREBUILT_ROOT}/lib/libQt5PrintSupport.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt5PrintSupport.${QT_LIB_VERSION}.dylib;
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
      ${PREBUILT_ROOT}/plugins/imageformats/libqwebp.dylib>lib64/qt/plugins/imageformats/libqwebp.dylib)

  # Note: this will only set the property for install targets, not during build.
  set(
    QT5_SHARED_PROPERTIES
    "INSTALL_RPATH>=@loader_path/lib64/qt/lib;INSTALL_RPATH>=@loader_path/lib64/qt/plugins;BUILD_WITH_INSTALL_RPATH=ON;LINK_FLAGS>=-framework Cocoa;INSTALL_RPATH_USE_LINK_PATH=ON"
    )
elseif(ANDROID_TARGET_OS STREQUAL "windows_msvc")
  # Clang/VS doesn't support linking directly to dlls. We linking to the import libraries instead (.lib).
  set(QT5_LIBRARIES -L${PREBUILT_ROOT}/lib)
  # Obtained by running ListDlls.exe from sysinternals tool
  set(QT5_SHARED_DEPENDENCIES
      ${PREBUILT_ROOT}/bin/Qt5Svg.dll>lib64/qt/lib/Qt5Svg.dll;
      ${PREBUILT_ROOT}/bin/Qt5Core.dll>lib64/qt/lib/Qt5Core.dll;
      ${PREBUILT_ROOT}/bin/Qt5Gui.dll>lib64/qt/lib/Qt5Gui.dll;
      ${PREBUILT_ROOT}/bin/Qt5Widgets.dll>lib64/qt/lib/Qt5Widgets.dll;
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
      ${PREBUILT_ROOT}/plugins/imageformats/qwebp.dll>lib64/qt/plugins/imageformats/qwebp.dll;)
elseif(ANDROID_TARGET_TAG MATCHES "windows-x86.*")
  # On Windows, linking to mingw32 is required. The library is provided by the toolchain, and depends on a main()
  # function provided by qtmain which itself depends on qMain(). These must appear iemulator-libui_unittestsn LDFLAGS
  # and not LDLIBS since qMain() is provided by object/libraries that appear after these in the link command-line.
  set(QT5_LIBRARIES -L${PREBUILT_ROOT}/bin -lmingw32 ${PREBUILT_ROOT}/lib/libqtmain.a)
  # Obtained by running ListDlls.exe from sysinternals tool
  set(QT5_SHARED_DEPENDENCIES
      ${PREBUILT_ROOT}/bin/Qt5Svg.dll>lib64/qt/lib/Qt5Svg.dll;
      ${PREBUILT_ROOT}/bin/Qt5Core.dll>lib64/qt/lib/Qt5Core.dll;
      ${PREBUILT_ROOT}/bin/Qt5Gui.dll>lib64/qt/lib/Qt5Gui.dll;
      ${PREBUILT_ROOT}/bin/Qt5Widgets.dll>lib64/qt/lib/Qt5Widgets.dll;
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
      ${PREBUILT_ROOT}/plugins/imageformats/qwebp.dll>lib64/qt/plugins/imageformats/qwebp.dll;)
elseif(ANDROID_TARGET_TAG STREQUAL "linux-x86_64")
  set(QT5_LIBRARIES -L${PREBUILT_ROOT}/lib ${QT5_LIBRARIES})
  # LD_DEBUG=libs ./emulator @P_64 2>&1 | grep qt | grep init
  set(
    QT5_SHARED_DEPENDENCIES
    ${PREBUILT_ROOT}/lib/libQt5Core.so.5>lib64/qt//lib/libQt5Core.so.5;
    ${PREBUILT_ROOT}/lib/libQt5Gui.so.5>lib64/qt//lib/libQt5Gui.so.5;
    ${PREBUILT_ROOT}/lib/libQt5Widgets.so.5>lib64/qt//lib/libQt5Widgets.so.5;
    ${PREBUILT_ROOT}/lib/libQt5Svg.so.5>lib64/qt//lib/libQt5Svg.so.5;
    ${PREBUILT_ROOT}/lib/libQt5DBus.so.5>lib64/qt//lib/libQt5DBus.so.5;
    ${PREBUILT_ROOT}/lib/libQt5XcbQpa.so.5>lib64/qt//lib/libQt5XcbQpa.so.5;
    ${PREBUILT_ROOT}/plugins/platforms/libqxcb.so>lib64/qt//plugins/platforms/libqxcb.so;
    ${PREBUILT_ROOT}/plugins/xcbglintegrations/libqxcb-glx-integration.so>lib64/qt//plugins/xcbglintegrations/libqxcb-glx-integration.so;
    ${PREBUILT_ROOT}/plugins/platforminputcontexts/libcomposeplatforminputcontextplugin.so>lib64/qt//plugins/platforminputcontexts/libcomposeplatforminputcontextplugin.so;
    ${PREBUILT_ROOT}/plugins/iconengines/libqsvgicon.so>lib64/qt//plugins/iconengines/libqsvgicon.so;
    ${PREBUILT_ROOT}/plugins/imageformats/libqwebp.so>lib64/qt//plugins/imageformats/libqwebp.so;
    ${PREBUILT_ROOT}/plugins/imageformats/libqgif.so>lib64/qt//plugins/imageformats/libqgif.so;
    ${PREBUILT_ROOT}/plugins/imageformats/libqicns.so>lib64/qt//plugins/imageformats/libqicns.so;
    ${PREBUILT_ROOT}/plugins/imageformats/libqico.so>lib64/qt//plugins/imageformats/libqico.so;
    ${PREBUILT_ROOT}/plugins/imageformats/libqjpeg.so>lib64/qt//plugins/imageformats/libqjpeg.so;
    ${PREBUILT_ROOT}/plugins/imageformats/libqsvg.so>lib64/qt//plugins/imageformats/libqsvg.so;
    ${PREBUILT_ROOT}/plugins/imageformats/libqtga.so>lib64/qt//plugins/imageformats/libqtga.so;
    ${PREBUILT_ROOT}/plugins/imageformats/libqtiff.so>lib64/qt//plugins/imageformats/libqtiff.so;
    ${PREBUILT_ROOT}/plugins/imageformats/libqwbmp.so>lib64/qt//plugins/imageformats/libqwbmp.so)
  set(QT5_SHARED_PROPERTIES
      "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/qt/lib';LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/qt/lib/plugins'")
endif()

# Define Qt5 Compatible targets
if(NOT TARGET Qt5::Widgets)
  add_library(Qt5::Widgets INTERFACE IMPORTED GLOBAL)
  set_target_properties(Qt5::Widgets
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${QT5_INCLUDE_DIRS}"
                                   INTERFACE_LINK_LIBRARIES
                                   "-lQt5Widgets;${QT5_LIBRARIES}"
                                   INTERFACE_COMPILE_DEFINITIONS
                                   "QT5_STATICLIB")
  add_library(Qt5::Core INTERFACE IMPORTED GLOBAL)
  set_target_properties(Qt5::Core
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${QT5_INCLUDE_DIRS}"
                                   INTERFACE_LINK_LIBRARIES
                                   "-lQt5Core;${QT5_LIBRARIES}"
                                   INTERFACE_COMPILE_DEFINITIONS
                                   "QT5_STATICLIB")
  add_library(Qt5::Gui INTERFACE IMPORTED GLOBAL)
  set_target_properties(Qt5::Gui
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${QT5_INCLUDE_DIRS}"
                                   INTERFACE_LINK_LIBRARIES
                                   "-lQt5Gui;${QT5_LIBRARIES}"
                                   INTERFACE_COMPILE_DEFINITIONS
                                   "QT5_STATICLIB")
  add_library(Qt5::Svg INTERFACE IMPORTED GLOBAL)
  set_target_properties(Qt5::Svg
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${QT5_INCLUDE_DIRS}"
                                   INTERFACE_LINK_LIBRARIES
                                   "-lQt5Svg;${QT5_LIBRARIES}"
                                   INTERFACE_COMPILE_DEFINITIONS
                                   "QT5_STATICLIB")
endif()

set(QT5_LIBRARIES ${QT5_LIBRARIES} -lQt5Widgets -lQt5Gui -lQt5Core -lQt5Svg)

set(
  PACKAGE_EXPORT
  "QT5_INCLUDE_DIR;QT5_INCLUDE_DIRS;QT5_LIBRARIES;QT5_FOUND;QT5_DEFINITIONS;CMAKE_AUTOMOC;CMAKE_AUTOUIC;CMAKE_AUTORCC;QT_MOC_EXECUTABLE;QT_UIC_EXECUTABLE;QT_RCC_EXECUTABLE;QT_VERSION_MAJOR;QT5_SHARED_DEPENDENCIES;QT5_SHARED_PROPERTIES"
  )

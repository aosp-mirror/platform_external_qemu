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

# TODO: Remove this once we have -no-window working with absolutely no Qt dependencies
if(NOT QTWEBENGINE AND ANDROID_TARGET_TAG MATCHES "linux-x86_64")
  get_filename_component(
    PREBUILT_ROOT "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/${ANDROID_TARGET_TAG}-nowebengine"
    ABSOLUTE)

  # Use the host compatible moc, rcc and uic
  get_filename_component(
    HOST_PREBUILT_ROOT
    "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/${ANDROID_HOST_TAG}-nowebengine" ABSOLUTE)
else()
  get_filename_component(PREBUILT_ROOT
                         "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/${ANDROID_TARGET_TAG}"
                         ABSOLUTE)

  # Use the host compatible moc, rcc and uic
  get_filename_component(HOST_PREBUILT_ROOT
                         "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/${ANDROID_HOST_TAG}"
                         ABSOLUTE)
endif()

set(QT_VERSION_MAJOR 5)
set(QT_VERSION_MINOR 12)
set(CMAKE_AUTOMOC TRUE)
set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTORCC TRUE)

# Registers the uic/rcc/moc targets if needed..
function(add_qt_target target)
  if(MSVC)
    set(EXE_SUFFIX ".exe")
  endif()
  if(NOT TARGET Qt5::${target})
    set(QT_EXECUTABLE ${HOST_PREBUILT_ROOT}/bin/${target}${EXE_SUFFIX})
    if(NOT EXISTS ${QT_EXECUTABLE})
      message(FATAL_ERROR "Unable to add Qt5::${target} due to missing ${QT_EXECUTABLE}")
    endif()
    add_executable(Qt5::${target} IMPORTED GLOBAL)
    set_property(TARGET Qt5::${target} PROPERTY IMPORTED_LOCATION ${QT_EXECUTABLE})
  endif()
endfunction()

add_qt_target(uic)
add_qt_target(moc)
add_qt_target(rcc)

# Registers a Qt5 link target..
function(add_qt_shared_lib target link target_deps)
  if(NOT TARGET Qt5::${target})
    add_library(Qt5::${target} INTERFACE IMPORTED GLOBAL)
    if(QT5_LINK_PATH)
      set(link "${link} ${QT5_LINK_PATH}")
    endif()
    set_target_properties(Qt5::${target}
                          PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                     "${QT5_INCLUDE_DIRS}"
                                     INTERFACE_LINK_LIBRARIES
                                     "${link}"
                                     INTERFACE_COMPILE_DEFINITIONS
                                     "QT5_STATICLIB")
    # Update the dependencies
    foreach(dep ${target_deps})
      target_link_libraries(Qt5::${target} INTERFACE ${dep})
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

if(DARWIN_X86_64)
  set(QT_LIB_VERSION 5.12.1)
  set(QT5_LIBRARIES -L${PREBUILT_ROOT}/lib ${QT5_LIBRARIES})
  set(
    QT5_SHARED_DEPENDENCIES
    ${PREBUILT_ROOT}/lib/libQt5CoreAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt5CoreAndroidEmu.${QT_LIB_VERSION}.dylib;
    ${PREBUILT_ROOT}/lib/libQt5WidgetsAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt5WidgetsAndroidEmu.${QT_LIB_VERSION}.dylib;
    ${PREBUILT_ROOT}/lib/libQt5GuiAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt5GuiAndroidEmu.${QT_LIB_VERSION}.dylib;
    ${PREBUILT_ROOT}/lib/libQt5SvgAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt5SvgAndroidEmu.${QT_LIB_VERSION}.dylib;
    ${PREBUILT_ROOT}/lib/libQt5PrintSupportAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt5PrintSupportAndroidEmu.${QT_LIB_VERSION}.dylib;
    ${PREBUILT_ROOT}/lib/libQt5NetworkAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt5NetworkAndroidEmu.${QT_LIB_VERSION}.dylib;
    ${PREBUILT_ROOT}/lib/libQt5DBusAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt5DBusAndroidEmu.${QT_LIB_VERSION}.dylib;
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

  add_qt_shared_lib(Core "-lQt5CoreAndroidEmu" "")
  add_qt_shared_lib(Gui "-lQt5GuiAndroidEmu" "Qt5::Core")
  add_qt_shared_lib(Widgets "-lQt5WidgetsAndroidEmu" "Qt5::Gui")
  add_qt_shared_lib(Svg "-lQt5SvgAndroidEmu" "Qt5::Widgets")

  if(QTWEBENGINE)
    add_qt_shared_lib(Network "-lQt5NetworkAndroidEmu" "Qt5::Core")
    add_qt_shared_lib(Qml "-lQt5QmlAndroidEmu" "Qt5::Network")
    add_qt_shared_lib(WebChannel "-lQt5WebChannelAndroidEmu" "Qt5::Qml")
    add_qt_shared_lib(WebSockets "-lQt5WebSocketsAndroidEmu" "Qt5::Qml")
    add_qt_shared_lib(WebEngineWidgets "-lQt5WebEngineWidgetsAndroidEmu" "Qt5::Qml")

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
        # TODO: These 6 are copies of the files immediately above. It would be nice to make
        # these symbolic links rather than copies.
        ${PREBUILT_ROOT}/resources/icudtl.dat>lib64/qt/libexec/icudtl.dat
        ${PREBUILT_ROOT}/resources/qtwebengine_devtools_resources.pak>lib64/qt/libexec/qtwebengine_devtools_resources.pak
        ${PREBUILT_ROOT}/resources/qtwebengine_resources.pak>lib64/qt/libexec/qtwebengine_resources.pak
        ${PREBUILT_ROOT}/resources/qtwebengine_resources_100p.pak>lib64/qt/libexec/qtwebengine_resources_100p.pak
        ${PREBUILT_ROOT}/resources/qtwebengine_resources_200p.pak>lib64/qt/libexec/qtwebengine_resources_200p.pak
        ${PREBUILT_ROOT}/translations/qtwebengine_locales/*.pak>>lib64/qt/libexec/translations/qtwebengine_locales
        ${PREBUILT_ROOT}/lib/libQt5QmlAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt5QmlAndroidEmu.${QT_LIB_VERSION}.dylib;
        ${PREBUILT_ROOT}/lib/libQt5QuickAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt5QuickAndroidEmu.${QT_LIB_VERSION}.dylib;
        ${PREBUILT_ROOT}/lib/libQt5QuickWidgetsAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt5QuickWidgetsAndroidEmu.${QT_LIB_VERSION}.dylib;
        ${PREBUILT_ROOT}/lib/libQt5WebChannelAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt5WebChannelAndroidEmu.${QT_LIB_VERSION}.dylib;
        ${PREBUILT_ROOT}/lib/libQt5WebEngineCoreAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt5WebEngineCoreAndroidEmu.${QT_LIB_VERSION}.dylib;
        ${PREBUILT_ROOT}/lib/libQt5WebEngineWidgetsAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt5WebEngineWidgetsAndroidEmu.${QT_LIB_VERSION}.dylib;
        ${PREBUILT_ROOT}/lib/libQt5WebSocketsAndroidEmu.${QT_LIB_VERSION}.dylib>lib64/qt/lib/libQt5WebSocketsAndroidEmu.${QT_LIB_VERSION}.dylib
      )
  endif()

  # Note: this will only set the property for install targets, not during build.
  set(
    QT5_SHARED_PROPERTIES
    "INSTALL_RPATH>=@loader_path/lib64/qt/libexec;INSTALL_RPATH>=@loader_path/lib64/qt/lib;INSTALL_RPATH>=@loader_path/lib64/qt/plugins"
    )
  set(QT5_LIBRARIES ${QT5_LIBRARIES} -lQt5NetworkAndroidEmu -lQt5WebChannelAndroidEmu -lQt5WebEngineWidgetsAndroidEmu -lQt5WebSockets)
elseif(WINDOWS_MSVC_X86_64)
  # On Windows, Qt provides us with a qtmain.lib which helps us write a cross-platform main() function (because WinMain() is the entry point
  # to a GUI application on Windows). Exactly how qtmain does this is it defines WinMain() for us, and inside WinMain(), it calls a function
  # that we provide. For mingw, this function is qMain(), and for msvc, it's just main() (see qt-src/.../qtmain_lib.cpp for more info).
  # So, in short, for msvc,
  #     qtmain.lib --> WinMain() --> main() --> qt_main()
  # and for mingw,
  #     qtmain.a --> WinMain() --> qMain() --> qt_main()
  #
  # Clang/VS doesn't support linking directly to dlls. We linking to the import libraries instead (.lib).
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

  add_qt_shared_lib(Core "${PREBUILT_ROOT}/lib/Qt5CoreAndroidEmu.lib" "")
  add_qt_shared_lib(Gui "${PREBUILT_ROOT}/lib/Qt5GuiAndroidEmu.lib" "Qt5::Core")
  add_qt_shared_lib(Widgets "${PREBUILT_ROOT}/lib/Qt5WidgetsAndroidEmu.lib" "Qt5::Gui")
  add_qt_shared_lib(Svg "${PREBUILT_ROOT}/lib/Qt5SvgAndroidEmu.lib" "Qt5::Widgets")

  add_qt_shared_lib(Network "${PREBUILT_ROOT}/lib/Qt5NetworkAndroidEmu.lib" "Qt5::Core")
  add_qt_shared_lib(Qml "${PREBUILT_ROOT}/lib/Qt5QmlAndroidEmu.lib" "Qt5::Network")
  add_qt_shared_lib(WebEngineCore "${PREBUILT_ROOT}/lib/Qt5WebEngineCoreAndroidEmu.lib" "Qt5::Network")
  add_qt_shared_lib(WebChannel "${PREBUILT_ROOT}/lib/Qt5WebChannelAndroidEmu.lib" "Qt5::WebEngineCore;Qt5::Qml")
  add_qt_shared_lib(WebSockets "${PREBUILT_ROOT}/lib/Qt5WebSocketsAndroidEmu.lib" "Qt5::WebEngineCore;Qt5::Qml")
  add_qt_shared_lib(WebEngineWidgets "${PREBUILT_ROOT}/lib/Qt5WebEngineWidgetsAndroidEmu.lib" "Qt5::WebEngineCore;Qt5::Qml")

elseif(WINDOWS_X86_64)
  # On Windows, linking to mingw32 is required. The library is provided by the toolchain, and depends on a main()
  # function provided by qtmain which itself depends on qMain(). These must appear in emulator-libui_unittests LDFLAGS
  # and not LDLIBS since qMain() is provided by object/libraries that appear after these in the link command-line.
  set(QT5_LIBRARIES -L${PREBUILT_ROOT}/bin;-lmingw32;${PREBUILT_ROOT}/lib/libqtmainAndroidEmu.a)
  # Obtained by running ListDlls.exe from sysinternals tool
  set(QT5_SHARED_DEPENDENCIES
      ${PREBUILT_ROOT}/bin/Qt5SvgAndroidEmu.dll>lib64/qt/lib/Qt5SvgAndroidEmu.dll;
      ${PREBUILT_ROOT}/bin/Qt5CoreAndroidEmu.dll>lib64/qt/lib/Qt5CoreAndroidEmu.dll;
      ${PREBUILT_ROOT}/bin/Qt5GuiAndroidEmu.dll>lib64/qt/lib/Qt5GuiAndroidEmu.dll;
      ${PREBUILT_ROOT}/bin/Qt5WidgetsAndroidEmu.dll>lib64/qt/lib/Qt5WidgetsAndroidEmu.dll;
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
  add_qt_shared_lib(Core "-lQt5CoreAndroidEmu" "${QT5_LIBRARIES}")
  add_qt_shared_lib(Gui "-lQt5GuiAndroidEmu" "Qt5::Core")
  add_qt_shared_lib(Widgets "-lQt5WidgetsAndroidEmu" "Qt5::Gui")
  add_qt_shared_lib(Svg "-lQt5SvgAndroidEmu" "Qt5::Widgets")
elseif(LINUX_X86_64)
  set(QT5_LIBRARIES -L${PREBUILT_ROOT}/lib ${QT5_LIBRARIES})
  # LD_DEBUG=libs ./emulator @P_64 2>&1 | grep qt | grep init
  set(
    QT5_SHARED_DEPENDENCIES
    ${PREBUILT_ROOT}/lib/libQt5CoreAndroidEmu.so.5>lib64/qt/lib/libQt5CoreAndroidEmu.so.5;
    ${PREBUILT_ROOT}/lib/libQt5GuiAndroidEmu.so.5>lib64/qt/lib/libQt5GuiAndroidEmu.so.5;
    ${PREBUILT_ROOT}/lib/libQt5WidgetsAndroidEmu.so.5>lib64/qt/lib/libQt5WidgetsAndroidEmu.so.5;
    ${PREBUILT_ROOT}/lib/libQt5SvgAndroidEmu.so.5>lib64/qt/lib/libQt5SvgAndroidEmu.so.5;
    ${PREBUILT_ROOT}/lib/libQt5PrintSupportAndroidEmu.so.5>lib64/qt/lib/libQt5PrintSupportAndroidEmu.so.5;
    ${PREBUILT_ROOT}/lib/libQt5DBusAndroidEmu.so.5>lib64/qt/lib/libQt5DBusAndroidEmu.so.5;
    ${PREBUILT_ROOT}/lib/libQt5NetworkAndroidEmu.so.5>lib64/qt/lib/libQt5NetworkAndroidEmu.so.5;
    ${PREBUILT_ROOT}/lib/libQt5XcbQpaAndroidEmu.so.5>lib64/qt/lib/libQt5XcbQpaAndroidEmu.so.5;
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
    ${PREBUILT_ROOT}/plugins/imageformats/libqwbmp.so>lib64/qt/plugins/imageformats/libqwbmp.so)
  set(QT5_SHARED_PROPERTIES
      "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/qt/lib';LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/qt/lib/plugins'")
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
      add_qt_shared_lib(WebEngineWidgets "-lQt5WebEngineWidgetsAndroidEmu" "Qt5::Qml")

    list(APPEND QT5_LIBRARIES ${QT5_LIBRARIES} -lQt5WebChannelAndroidEmu -lQt5WebEngineWidgetsAndroidEmu -lQt5WebSocketsAndroidEmu)
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
        # TODO: These six are copies of the files immediately above. It would be nice
        # to make these symbolic links rather than copies.
        ${PREBUILT_ROOT}/resources/icudtl.dat>lib64/qt/libexec/icudtl.dat
        ${PREBUILT_ROOT}/resources/qtwebengine_devtools_resources.pak>lib64/qt/libexec/qtwebengine_devtools_resources.pak
        ${PREBUILT_ROOT}/resources/qtwebengine_resources.pak>lib64/qt/libexec/qtwebengine_resources.pak
        ${PREBUILT_ROOT}/resources/qtwebengine_resources_100p.pak>lib64/qt/libexec/qtwebengine_resources_100p.pak
        ${PREBUILT_ROOT}/resources/qtwebengine_resources_200p.pak>lib64/qt/libexec/qtwebengine_resources_200p.pak
        ${PREBUILT_ROOT}/translations/qtwebengine_locales/*.pak>>lib64/qt/libexec/translations/qtwebengine_locales
        ${PREBUILT_ROOT}/lib/libQt5QmlAndroidEmu.so.5>lib64/qt/lib/libQt5QmlAndroidEmu.so.5;
        ${PREBUILT_ROOT}/lib/libQt5QuickAndroidEmu.so.5>lib64/qt/lib/libQt5QuickAndroidEmu.so.5;
        ${PREBUILT_ROOT}/lib/libQt5QuickWidgetsAndroidEmu.so.5>lib64/qt/lib/libQt5QuickWidgetsAndroidEmu.so.5;
        ${PREBUILT_ROOT}/lib/libQt5WebChannelAndroidEmu.so.5>lib64/qt/lib/libQt5WebChannelAndroidEmu.so.5;
        ${PREBUILT_ROOT}/lib/libQt5WebEngineCoreAndroidEmu.so.5>lib64/qt/lib/libQt5WebEngineCoreAndroidEmu.so.5;
        ${PREBUILT_ROOT}/lib/libQt5WebEngineWidgetsAndroidEmu.so.5>lib64/qt/lib/libQt5WebEngineWidgetsAndroidEmu.so.5;
        ${PREBUILT_ROOT}/lib/libQt5WebSocketsAndroidEmu.so.5>lib64/qt/lib/libQt5WebSocketsAndroidEmu.so.5)

    list(APPEND QT5_SHARED_PROPERTIES "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/qt/libexec'")
  endif()

  list(
      APPEND
        QT5_SHARED_DEPENDENCIES
        ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libxkbcommon.so.0.0.0>lib64/qt/lib/libxkbcommon.so;
        ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libxkbcommon.so.0.0.0>lib64/qt/lib/libxkbcommon.so.0;
        ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libxkbcommon.so.0.0.0>lib64/qt/lib/libxkbcommon.so.0.0.0;
        ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libsoftokn3.so>lib64/qt/lib/libsoftokn3.so;
        ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libsqlite3.so>lib64/qt/lib/libsqlite3.so;
        ${PREBUILT_WEBENGINE_DEPS_ROOT}/lib/libfreetype.so.6>lib64/qt/lib/libfreetype.so.6)

endif()

set(QT5_LIBRARIES ${QT5_LIBRARIES} -lQt5WidgetsAndroidEmu -lQt5GuiAndroidEmu -lQt5CoreAndroidEmu -lQt5SvgAndroidEmu)

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

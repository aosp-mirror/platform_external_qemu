# QT Settings ##
get_filename_component(
  PREBUILT_ROOT
  "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/${ANDROID_TARGET_TAG}"
  ABSOLUTE)

if(NOT TARGET Qt5::moc)
  set(QT_VERSION_MAJOR 5)
  set(CMAKE_AUTOMOC TRUE)
  set(CMAKE_AUTOUIC TRUE)
  set(CMAKE_AUTORCC TRUE)
  set(QT_MOC_EXECUTABLE ${PREBUILT_ROOT}/bin/moc)
  set(QT_UIC_EXECUTABLE ${PREBUILT_ROOT}/bin/uic)
  set(QT_RCC_EXECUTABLE ${PREBUILT_ROOT}/bin/rcc)
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
set(QT5_LIBRARIES -lQt5Widgets -lQt5Gui -lQt5Core -lQt5Svg)

set(QT5_CFLAGS "-DQT5_STATICLIB")
set(QT5_FOUND TRUE)

if(ANDROID_TARGET_TAG STREQUAL "darwin-x86_64")
  set(QT5_LIBRARIES -L${PREBUILT_ROOT}/lib ${QT5_LIBRARIES})
  set(
    QT5_SHARED_DEPENDENCIES
    ${PREBUILT_ROOT}/plugins/platforms/libqoffscreen.dylib>lib64/qt/plugins/platforms/libqoffscreen.dylib
    ${PREBUILT_ROOT}/plugins/platforms/libqminimal.dylib>lib64/qt/plugins/platforms/libqminimal.dylib
    ${PREBUILT_ROOT}/plugins/platforms/libqcocoa.dylib>lib64/qt/plugins/platforms/libqcocoa.dylib
    ${PREBUILT_ROOT}/plugins/printsupport>lib64/qt/plugins/printsupport
    ${PREBUILT_ROOT}/plugins/printsupport/libcocoaprintersupport.dylib>lib64/qt/plugins/printsupport/libcocoaprintersupport.dylib
    ${PREBUILT_ROOT}/plugins/bearer>lib64/qt/plugins/bearer
    ${PREBUILT_ROOT}/plugins/bearer/libqcorewlanbearer.dylib>lib64/qt/plugins/bearer/libqcorewlanbearer.dylib
    ${PREBUILT_ROOT}/plugins/bearer/libqgenericbearer.dylib>lib64/qt/plugins/bearer/libqgenericbearer.dylib
    ${PREBUILT_ROOT}/plugins/iconengines>lib64/qt/plugins/iconengines
    ${PREBUILT_ROOT}/plugins/iconengines/libqsvgicon.dylib>lib64/qt/plugins/iconengines/libqsvgicon.dylib
    ${PREBUILT_ROOT}/plugins/sqldrivers>lib64/qt/plugins/sqldrivers
    ${PREBUILT_ROOT}/plugins/sqldrivers/libqsqlite.dylib>lib64/qt/plugins/sqldrivers/libqsqlite.dylib
    ${PREBUILT_ROOT}/plugins/imageformats>lib64/qt/plugins/imageformats
    ${PREBUILT_ROOT}/plugins/imageformats/libqgif.dylib>lib64/qt/plugins/imageformats/libqgif.dylib
    ${PREBUILT_ROOT}/plugins/imageformats/libqwbmp.dylib>lib64/qt/plugins/imageformats/libqwbmp.dylib
    ${PREBUILT_ROOT}/plugins/imageformats/libqwebp.dylib>lib64/qt/plugins/imageformats/libqwebp.dylib
    ${PREBUILT_ROOT}/plugins/imageformats/libqdds.dylib>lib64/qt/plugins/imageformats/libqdds.dylib
    ${PREBUILT_ROOT}/plugins/imageformats/libqico.dylib>lib64/qt/plugins/imageformats/libqico.dylib
    ${PREBUILT_ROOT}/plugins/imageformats/libqjpeg.dylib>lib64/qt/plugins/imageformats/libqjpeg.dylib
    ${PREBUILT_ROOT}/plugins/imageformats/libqtiff.dylib>lib64/qt/plugins/imageformats/libqtiff.dylib
    ${PREBUILT_ROOT}/plugins/imageformats/libqsvg.dylib>lib64/qt/plugins/imageformats/libqsvg.dylib
    ${PREBUILT_ROOT}/plugins/imageformats/libqicns.dylib>lib64/qt/plugins/imageformats/libqicns.dylib
    ${PREBUILT_ROOT}/plugins/imageformats/libqtga.dylib>lib64/qt/plugins/imageformats/libqtga.dylib
    ${PREBUILT_ROOT}/plugins/imageformats/libqmacjp2.dylib>lib64/qt/plugins/imageformats/libqmacjp2.dylib
    ${PREBUILT_ROOT}/plugins/generic>lib64/qt/plugins/generic
    ${PREBUILT_ROOT}/plugins/generic/libqtuiotouchplugin.dylib>lib64/qt/plugins/generic/libqtuiotouchplugin.dylib
    ${PREBUILT_ROOT}/lib>lib64/qt/lib
    ${PREBUILT_ROOT}/lib/libQt5PrintSupport.dylib>lib64/qt/lib/libQt5PrintSupport.dylib
    ${PREBUILT_ROOT}/lib/libQt5Xml.5.7.0.dylib>lib64/qt/lib/libQt5Xml.5.7.0.dylib
    ${PREBUILT_ROOT}/lib/libQt5Network.5.7.dylib>lib64/qt/lib/libQt5Network.5.7.dylib
    ${PREBUILT_ROOT}/lib/libQt5Sql.5.7.dylib>lib64/qt/lib/libQt5Sql.5.7.dylib
    ${PREBUILT_ROOT}/lib/libQt5Widgets.5.7.dylib>lib64/qt/lib/libQt5Widgets.5.7.dylib
    ${PREBUILT_ROOT}/lib/libQt5PrintSupport.5.7.dylib>lib64/qt/lib/libQt5PrintSupport.5.7.dylib
    ${PREBUILT_ROOT}/lib/libQt5Xml.5.dylib>lib64/qt/lib/libQt5Xml.5.dylib
    ${PREBUILT_ROOT}/lib/libQt5Network.5.dylib>lib64/qt/lib/libQt5Network.5.dylib
    ${PREBUILT_ROOT}/lib/libQt5DBus.5.dylib>lib64/qt/lib/libQt5DBus.5.dylib
    ${PREBUILT_ROOT}/lib/libQt5DBus.5.7.0.dylib>lib64/qt/lib/libQt5DBus.5.7.0.dylib
    ${PREBUILT_ROOT}/lib/libQt5Gui.5.7.0.dylib>lib64/qt/lib/libQt5Gui.5.7.0.dylib
    ${PREBUILT_ROOT}/lib/libQt5Sql.5.7.0.dylib>lib64/qt/lib/libQt5Sql.5.7.0.dylib
    ${PREBUILT_ROOT}/lib/libQt5Widgets.dylib>lib64/qt/lib/libQt5Widgets.dylib
    ${PREBUILT_ROOT}/lib/libQt5Svg.5.7.dylib>lib64/qt/lib/libQt5Svg.5.7.dylib
    ${PREBUILT_ROOT}/lib/libQt5Test.5.dylib>lib64/qt/lib/libQt5Test.5.dylib
    ${PREBUILT_ROOT}/lib/libQt5Core.5.dylib>lib64/qt/lib/libQt5Core.5.dylib
    ${PREBUILT_ROOT}/lib/libQt5OpenGL.5.7.dylib>lib64/qt/lib/libQt5OpenGL.5.7.dylib
    ${PREBUILT_ROOT}/lib/libQt5PrintSupport.5.dylib>lib64/qt/lib/libQt5PrintSupport.5.dylib
    ${PREBUILT_ROOT}/lib/libQt5Test.5.7.0.dylib>lib64/qt/lib/libQt5Test.5.7.0.dylib
    ${PREBUILT_ROOT}/lib/libQt5Core.5.7.0.dylib>lib64/qt/lib/libQt5Core.5.7.0.dylib
    ${PREBUILT_ROOT}/lib/libQt5Test.5.7.dylib>lib64/qt/lib/libQt5Test.5.7.dylib
    ${PREBUILT_ROOT}/lib/libQt5Core.dylib>lib64/qt/lib/libQt5Core.dylib
    ${PREBUILT_ROOT}/lib/libQt5Concurrent.5.dylib>lib64/qt/lib/libQt5Concurrent.5.dylib
    ${PREBUILT_ROOT}/lib/libQt5Concurrent.dylib>lib64/qt/lib/libQt5Concurrent.dylib
    ${PREBUILT_ROOT}/lib/libQt5Gui.dylib>lib64/qt/lib/libQt5Gui.dylib
    ${PREBUILT_ROOT}/lib/libQt5Svg.5.dylib>lib64/qt/lib/libQt5Svg.5.dylib
    ${PREBUILT_ROOT}/lib/libQt5Xml.5.7.dylib>lib64/qt/lib/libQt5Xml.5.7.dylib
    ${PREBUILT_ROOT}/lib/libQt5Core.5.7.dylib>lib64/qt/lib/libQt5Core.5.7.dylib
    ${PREBUILT_ROOT}/lib/libQt5Svg.5.7.0.dylib>lib64/qt/lib/libQt5Svg.5.7.0.dylib
    ${PREBUILT_ROOT}/lib/libQt5Sql.5.dylib>lib64/qt/lib/libQt5Sql.5.dylib
    ${PREBUILT_ROOT}/lib/libQt5Concurrent.5.7.dylib>lib64/qt/lib/libQt5Concurrent.5.7.dylib
    ${PREBUILT_ROOT}/lib/libQt5PrintSupport.5.7.0.dylib>lib64/qt/lib/libQt5PrintSupport.5.7.0.dylib
    ${PREBUILT_ROOT}/lib/libQt5DBus.dylib>lib64/qt/lib/libQt5DBus.dylib
    ${PREBUILT_ROOT}/lib/libQt5Network.5.7.0.dylib>lib64/qt/lib/libQt5Network.5.7.0.dylib
    ${PREBUILT_ROOT}/lib/libQt5Xml.dylib>lib64/qt/lib/libQt5Xml.dylib
    ${PREBUILT_ROOT}/lib/libQt5Widgets.5.dylib>lib64/qt/lib/libQt5Widgets.5.dylib
    ${PREBUILT_ROOT}/lib/libQt5Gui.5.7.dylib>lib64/qt/lib/libQt5Gui.5.7.dylib
    ${PREBUILT_ROOT}/lib/libQt5DBus.5.7.dylib>lib64/qt/lib/libQt5DBus.5.7.dylib
    ${PREBUILT_ROOT}/lib/libQt5Concurrent.5.7.0.dylib>lib64/qt/lib/libQt5Concurrent.5.7.0.dylib
    ${PREBUILT_ROOT}/lib/libQt5OpenGL.5.7.0.dylib>lib64/qt/lib/libQt5OpenGL.5.7.0.dylib
    ${PREBUILT_ROOT}/lib/libQt5Sql.dylib>lib64/qt/lib/libQt5Sql.dylib
    ${PREBUILT_ROOT}/lib/libQt5OpenGL.dylib>lib64/qt/lib/libQt5OpenGL.dylib
    ${PREBUILT_ROOT}/lib/libQt5Test.dylib>lib64/qt/lib/libQt5Test.dylib
    ${PREBUILT_ROOT}/lib/libQt5Widgets.5.7.0.dylib>lib64/qt/lib/libQt5Widgets.5.7.0.dylib
    ${PREBUILT_ROOT}/lib/libQt5Svg.dylib>lib64/qt/lib/libQt5Svg.dylib
    ${PREBUILT_ROOT}/lib/libQt5Network.dylib>lib64/qt/lib/libQt5Network.dylib
    ${PREBUILT_ROOT}/lib/libQt5Gui.5.dylib>lib64/qt/lib/libQt5Gui.5.dylib
    ${PREBUILT_ROOT}/lib/libQt5OpenGL.5.dylib>lib64/qt/lib/libQt5OpenGL.5.dylib)
  # Note: this will only set the property for install targets, not during build.
  set(QT5_SHARED_PROPERTIES "INSTALL_RPATH>=@loader_path/lib64/qt/lib;BUILD_WITH_INSTALL_RPATH=ON;LINK_FLAGS>=-framework Cocoa;")

elseif(ANDROID_TARGET_TAG STREQUAL "windows-x86")
  # On Windows, linking to mingw32 is required. The library is provided by the
  # toolchain, and depends on a main() function provided by qtmain which itself
  # depends on qMain(). These must appear in LDFLAGS and not LDLIBS since
  # qMain() is provided by object/libraries that appear after these in the link
  # command-line.
  set(QT5_LIBRARIES
      -L${PREBUILT_ROOT}/bin
      ${QT5_LIBRARIES}
      -lmingw32
      ${PREBUILT_ROOT}/lib/libqtmain.a)
  set(
    QT5_SHARED_DEPENDENCIES
    ${PREBUILT_ROOT}/bin/Qt5Xml.dll>/lib/qt/lib/Qt5Xml.dll
    ${PREBUILT_ROOT}/bin/Qt5Test.dll>/lib/qt/lib/Qt5Test.dll
    ${PREBUILT_ROOT}/bin/Qt5DBus.dll>/lib/qt/lib/Qt5DBus.dll
    ${PREBUILT_ROOT}/bin/Qt5Concurrent.dll>/lib/qt/lib/Qt5Concurrent.dll
    ${PREBUILT_ROOT}/bin/Qt5Network.dll>/lib/qt/lib/Qt5Network.dll
    ${PREBUILT_ROOT}/bin/Qt5Gui.dll>/lib/qt/lib/Qt5Gui.dll
    ${PREBUILT_ROOT}/bin/Qt5Core.dll>/lib/qt/lib/Qt5Core.dll
    ${PREBUILT_ROOT}/bin/Qt5Widgets.dll>/lib/qt/lib/Qt5Widgets.dll
    ${PREBUILT_ROOT}/bin/Qt5OpenGL.dll>/lib/qt/lib/Qt5OpenGL.dll
    ${PREBUILT_ROOT}/bin/Qt5PrintSupport.dll>/lib/qt/lib/Qt5PrintSupport.dll
    ${PREBUILT_ROOT}/bin/Qt5Svg.dll>/lib/qt/lib/Qt5Svg.dll
    ${PREBUILT_ROOT}/bin/Qt5Sql.dll>/lib/qt/lib/Qt5Sql.dll
    ${PREBUILT_ROOT}/plugins/iconengines/qsvgicon.dll>/lib/qt/plugins/iconengines/qsvgicon.dll
    ${PREBUILT_ROOT}/plugins/sqldrivers/qsqlite.dll>/lib/qt/plugins/sqldrivers/qsqlite.dll
    ${PREBUILT_ROOT}/plugins/sqldrivers/qsqlodbc.dll>/lib/qt/plugins/sqldrivers/qsqlodbc.dll
    ${PREBUILT_ROOT}/plugins/generic/qtuiotouchplugin.dll>/lib/qt/plugins/generic/qtuiotouchplugin.dll
    ${PREBUILT_ROOT}/plugins/bearer/qgenericbearer.dll>/lib/qt/plugins/bearer/qgenericbearer.dll
    ${PREBUILT_ROOT}/plugins/bearer/qnativewifibearer.dll>/lib/qt/plugins/bearer/qnativewifibearer.dll
    ${PREBUILT_ROOT}/plugins/imageformats/qgif.dll>/lib/qt/plugins/imageformats/qgif.dll
    ${PREBUILT_ROOT}/plugins/imageformats/qico.dll>/lib/qt/plugins/imageformats/qico.dll
    ${PREBUILT_ROOT}/plugins/imageformats/qwebp.dll>/lib/qt/plugins/imageformats/qwebp.dll
    ${PREBUILT_ROOT}/plugins/imageformats/qtiff.dll>/lib/qt/plugins/imageformats/qtiff.dll
    ${PREBUILT_ROOT}/plugins/imageformats/qdds.dll>/lib/qt/plugins/imageformats/qdds.dll
    ${PREBUILT_ROOT}/plugins/imageformats/qjpeg.dll>/lib/qt/plugins/imageformats/qjpeg.dll
    ${PREBUILT_ROOT}/plugins/imageformats/qtga.dll>/lib/qt/plugins/imageformats/qtga.dll
    ${PREBUILT_ROOT}/plugins/imageformats/qsvg.dll>/lib/qt/plugins/imageformats/qsvg.dll
    ${PREBUILT_ROOT}/plugins/imageformats/qicns.dll>/lib/qt/plugins/imageformats/qicns.dll
    ${PREBUILT_ROOT}/plugins/imageformats/qwbmp.dll>/lib/qt/plugins/imageformats/qwbmp.dll
    ${PREBUILT_ROOT}/plugins/platforms/qwindows.dll>/lib/qt/plugins/platforms/qwindows.dll
    ${PREBUILT_ROOT}/plugins/platforms/qminimal.dll>/lib/qt/plugins/platforms/qminimal.dll
    ${PREBUILT_ROOT}/plugins/printsupport/windowsprintersupport.dll>/lib/qt/plugins/printsupport/windowsprintersupport.dll
    )
elseif(ANDROID_TARGET_TAG STREQUAL "windows-x86_64")
  # On Windows, linking to mingw32 is required. The library is provided by the
  # toolchain, and depends on a main() function provided by qtmain which itself
  # depends on qMain(). These must appear in LDFLAGS and not LDLIBS since
  # qMain() is provided by object/libraries that appear after these in the link
  # command-line.
  set(QT5_LIBRARIES
      -L${PREBUILT_ROOT}/bin
      ${QT5_LIBRARIES}
      -lmingw32
      ${PREBUILT_ROOT}/lib/libqtmain.a)
  set(
    QT5_SHARED_DEPENDENCIES
    ${PREBUILT_ROOT}/bin/Qt5Xml.dll>/lib64/qt/lib64/Qt5Xml.dll
    ${PREBUILT_ROOT}/bin/Qt5Test.dll>/lib64/qt/lib64/Qt5Test.dll
    ${PREBUILT_ROOT}/bin/Qt5DBus.dll>/lib64/qt/lib64/Qt5DBus.dll
    ${PREBUILT_ROOT}/bin/Qt5Concurrent.dll>/lib64/qt/lib64/Qt5Concurrent.dll
    ${PREBUILT_ROOT}/bin/Qt5Network.dll>/lib64/qt/lib64/Qt5Network.dll
    ${PREBUILT_ROOT}/bin/Qt5Gui.dll>/lib64/qt/lib64/Qt5Gui.dll
    ${PREBUILT_ROOT}/bin/Qt5Core.dll>/lib64/qt/lib64/Qt5Core.dll
    ${PREBUILT_ROOT}/bin/Qt5Widgets.dll>/lib64/qt/lib64/Qt5Widgets.dll
    ${PREBUILT_ROOT}/bin/Qt5OpenGL.dll>/lib64/qt/lib64/Qt5OpenGL.dll
    ${PREBUILT_ROOT}/bin/Qt5PrintSupport.dll>/lib64/qt/lib64/Qt5PrintSupport.dll
    ${PREBUILT_ROOT}/bin/Qt5Svg.dll>/lib64/qt/lib64/Qt5Svg.dll
    ${PREBUILT_ROOT}/bin/Qt5Sql.dll>/lib64/qt/lib64/Qt5Sql.dll
    ${PREBUILT_ROOT}/plugins/iconengines/qsvgicon.dll>/lib64/qt/plugins/iconengines/qsvgicon.dll
    ${PREBUILT_ROOT}/plugins/sqldrivers/qsqlite.dll>/lib64/qt/plugins/sqldrivers/qsqlite.dll
    ${PREBUILT_ROOT}/plugins/sqldrivers/qsqlodbc.dll>/lib64/qt/plugins/sqldrivers/qsqlodbc.dll
    ${PREBUILT_ROOT}/plugins/generic/qtuiotouchplugin.dll>/lib64/qt/plugins/generic/qtuiotouchplugin.dll
    ${PREBUILT_ROOT}/plugins/bearer/qgenericbearer.dll>/lib64/qt/plugins/bearer/qgenericbearer.dll
    ${PREBUILT_ROOT}/plugins/bearer/qnativewifibearer.dll>/lib64/qt/plugins/bearer/qnativewifibearer.dll
    ${PREBUILT_ROOT}/plugins/imageformats/qgif.dll>/lib64/qt/plugins/imageformats/qgif.dll
    ${PREBUILT_ROOT}/plugins/imageformats/qico.dll>/lib64/qt/plugins/imageformats/qico.dll
    ${PREBUILT_ROOT}/plugins/imageformats/qwebp.dll>/lib64/qt/plugins/imageformats/qwebp.dll
    ${PREBUILT_ROOT}/plugins/imageformats/qtiff.dll>/lib64/qt/plugins/imageformats/qtiff.dll
    ${PREBUILT_ROOT}/plugins/imageformats/qdds.dll>/lib64/qt/plugins/imageformats/qdds.dll
    ${PREBUILT_ROOT}/plugins/imageformats/qjpeg.dll>/lib64/qt/plugins/imageformats/qjpeg.dll
    ${PREBUILT_ROOT}/plugins/imageformats/qtga.dll>/lib64/qt/plugins/imageformats/qtga.dll
    ${PREBUILT_ROOT}/plugins/imageformats/qsvg.dll>/lib64/qt/plugins/imageformats/qsvg.dll
    ${PREBUILT_ROOT}/plugins/imageformats/qicns.dll>/lib64/qt/plugins/imageformats/qicns.dll
    ${PREBUILT_ROOT}/plugins/imageformats/qwbmp.dll>/lib64/qt/plugins/imageformats/qwbmp.dll
    ${PREBUILT_ROOT}/plugins/platforms/qwindows.dll>/lib64/qt/plugins/platforms/qwindows.dll
    ${PREBUILT_ROOT}/plugins/platforms/qminimal.dll>/lib64/qt/plugins/platforms/qminimal.dll
    ${PREBUILT_ROOT}/plugins/printsupport/windowsprintersupport.dll>/lib64/qt/plugins/printsupport/windowsprintersupport.dll
    )
elseif(ANDROID_TARGET_TAG STREQUAL "linux-x86_64")
  set(QT5_LIBRARIES -L${PREBUILT_ROOT}/lib ${QT5_LIBRARIES})
  set(
    QT5_SHARED_DEPENDENCIES
    ${PREBUILT_ROOT}/lib/libQt5DBus.so.5.7.0>lib64/qt/lib/libQt5DBus.so.5.7.0
    ${PREBUILT_ROOT}/lib/libQt5Xml.so.5.7.0>lib64/qt/lib/libQt5Xml.so.5.7.0
    ${PREBUILT_ROOT}/lib/libQt5Svg.so.5.7>lib64/qt/lib/libQt5Svg.so.5.7
    ${PREBUILT_ROOT}/lib/libQt5OpenGL.so.5.7>lib64/qt/lib/libQt5OpenGL.so.5.7
    ${PREBUILT_ROOT}/lib/libQt5Concurrent.so>lib64/qt/lib/libQt5Concurrent.so
    ${PREBUILT_ROOT}/lib/libQt5Sql.so.5>lib64/qt/lib/libQt5Sql.so.5
    ${PREBUILT_ROOT}/lib/libQt5Svg.so>lib64/qt/lib/libQt5Svg.so
    ${PREBUILT_ROOT}/lib/libQt5Network.so.5>lib64/qt/lib/libQt5Network.so.5
    ${PREBUILT_ROOT}/lib/libQt5Sql.so>lib64/qt/lib/libQt5Sql.so
    ${PREBUILT_ROOT}/lib/libQt5Test.so>lib64/qt/lib/libQt5Test.so
    ${PREBUILT_ROOT}/lib/libQt5Test.so.5>lib64/qt/lib/libQt5Test.so.5
    ${PREBUILT_ROOT}/lib/libQt5Xml.so.5.7>lib64/qt/lib/libQt5Xml.so.5.7
    ${PREBUILT_ROOT}/lib/libQt5Concurrent.so.5>lib64/qt/lib/libQt5Concurrent.so.5
    ${PREBUILT_ROOT}/lib/libQt5Core.so.5.7.0>lib64/qt/lib/libQt5Core.so.5.7.0
    ${PREBUILT_ROOT}/lib/libQt5Sql.so.5.7>lib64/qt/lib/libQt5Sql.so.5.7
    ${PREBUILT_ROOT}/lib/libQt5Network.so.5.7>lib64/qt/lib/libQt5Network.so.5.7
    ${PREBUILT_ROOT}/lib/libQt5Svg.so.5.7.0>lib64/qt/lib/libQt5Svg.so.5.7.0
    ${PREBUILT_ROOT}/lib/libQt5Concurrent.so.5.7>lib64/qt/lib/libQt5Concurrent.so.5.7
    ${PREBUILT_ROOT}/lib/libQt5Widgets.so.5.7.0>lib64/qt/lib/libQt5Widgets.so.5.7.0
    ${PREBUILT_ROOT}/lib/libQt5PrintSupport.so>lib64/qt/lib/libQt5PrintSupport.so
    ${PREBUILT_ROOT}/lib/libQt5XcbQpa.so>lib64/qt/lib/libQt5XcbQpa.so
    ${PREBUILT_ROOT}/lib/libQt5Sql.so.5.7.0>lib64/qt/lib/libQt5Sql.so.5.7.0
    ${PREBUILT_ROOT}/lib/libQt5Test.so.5.7.0>lib64/qt/lib/libQt5Test.so.5.7.0
    ${PREBUILT_ROOT}/lib/libQt5PrintSupport.so.5.7>lib64/qt/lib/libQt5PrintSupport.so.5.7
    ${PREBUILT_ROOT}/lib/libQt5Svg.so.5>lib64/qt/lib/libQt5Svg.so.5
    ${PREBUILT_ROOT}/lib/libQt5Widgets.so>lib64/qt/lib/libQt5Widgets.so
    ${PREBUILT_ROOT}/lib/libQt5Test.so.5.7>lib64/qt/lib/libQt5Test.so.5.7
    ${PREBUILT_ROOT}/lib/libQt5Core.so.5>lib64/qt/lib/libQt5Core.so.5
    ${PREBUILT_ROOT}/lib/libQt5XcbQpa.so.5.7>lib64/qt/lib/libQt5XcbQpa.so.5.7
    ${PREBUILT_ROOT}/lib/libQt5Xml.so>lib64/qt/lib/libQt5Xml.so
    ${PREBUILT_ROOT}/lib/libQt5Gui.so.5.7>lib64/qt/lib/libQt5Gui.so.5.7
    ${PREBUILT_ROOT}/lib/libQt5Core.so.5.7>lib64/qt/lib/libQt5Core.so.5.7
    ${PREBUILT_ROOT}/lib/libQt5Gui.so.5.7.0>lib64/qt/lib/libQt5Gui.so.5.7.0
    ${PREBUILT_ROOT}/lib/libQt5Network.so.5.7.0>lib64/qt/lib/libQt5Network.so.5.7.0
    ${PREBUILT_ROOT}/lib/libQt5Core.so>lib64/qt/lib/libQt5Core.so
    ${PREBUILT_ROOT}/lib/libQt5PrintSupport.so.5>lib64/qt/lib/libQt5PrintSupport.so.5
    ${PREBUILT_ROOT}/lib/libQt5Concurrent.so.5.7.0>lib64/qt/lib/libQt5Concurrent.so.5.7.0
    ${PREBUILT_ROOT}/lib/libQt5XcbQpa.so.5.7.0>lib64/qt/lib/libQt5XcbQpa.so.5.7.0
    ${PREBUILT_ROOT}/lib/libQt5Gui.so.5>lib64/qt/lib/libQt5Gui.so.5
    ${PREBUILT_ROOT}/lib/libQt5DBus.so>lib64/qt/lib/libQt5DBus.so
    ${PREBUILT_ROOT}/lib/libQt5Network.so>lib64/qt/lib/libQt5Network.so
    ${PREBUILT_ROOT}/lib/libQt5Widgets.so.5>lib64/qt/lib/libQt5Widgets.so.5
    ${PREBUILT_ROOT}/lib/libQt5XcbQpa.so.5>lib64/qt/lib/libQt5XcbQpa.so.5
    ${PREBUILT_ROOT}/lib/libQt5DBus.so.5.7>lib64/qt/lib/libQt5DBus.so.5.7
    ${PREBUILT_ROOT}/lib/libQt5Xml.so.5>lib64/qt/lib/libQt5Xml.so.5
    ${PREBUILT_ROOT}/lib/libQt5Gui.so>lib64/qt/lib/libQt5Gui.so
    ${PREBUILT_ROOT}/lib/libQt5Widgets.so.5.7>lib64/qt/lib/libQt5Widgets.so.5.7
    ${PREBUILT_ROOT}/lib/libQt5DBus.so.5>lib64/qt/lib/libQt5DBus.so.5
    ${PREBUILT_ROOT}/lib/libQt5OpenGL.so.5.7.0>lib64/qt/lib/libQt5OpenGL.so.5.7.0
    ${PREBUILT_ROOT}/lib/libc++.so>lib64/qt/lib/libc++.so
    ${PREBUILT_ROOT}/lib/libQt5OpenGL.so>lib64/qt/lib/libQt5OpenGL.so
    ${PREBUILT_ROOT}/lib/libQt5OpenGL.so.5>lib64/qt/lib/libQt5OpenGL.so.5
    ${PREBUILT_ROOT}/lib/libQt5PrintSupport.so.5.7.0>lib64/qt/lib/libQt5PrintSupport.so.5.7.0
    ${PREBUILT_ROOT}/plugins/iconengines/libqsvgicon.so>lib64/qt/plugins/iconengines/libqsvgicon.so
    ${PREBUILT_ROOT}/plugins/sqldrivers/libqsqlite.so>lib64/qt/plugins/sqldrivers/libqsqlite.so
    ${PREBUILT_ROOT}/plugins/platforminputcontexts/libibusplatforminputcontextplugin.so>lib64/qt/plugins/platforminputcontexts/libibusplatforminputcontextplugin.so
    ${PREBUILT_ROOT}/plugins/platforminputcontexts/libcomposeplatforminputcontextplugin.so>lib64/qt/plugins/platforminputcontexts/libcomposeplatforminputcontextplugin.so
    ${PREBUILT_ROOT}/plugins/generic/libqevdevmouseplugin.so>lib64/qt/plugins/generic/libqevdevmouseplugin.so
    ${PREBUILT_ROOT}/plugins/generic/libqevdevkeyboardplugin.so>lib64/qt/plugins/generic/libqevdevkeyboardplugin.so
    ${PREBUILT_ROOT}/plugins/generic/libqevdevtabletplugin.so>lib64/qt/plugins/generic/libqevdevtabletplugin.so
    ${PREBUILT_ROOT}/plugins/generic/libqtuiotouchplugin.so>lib64/qt/plugins/generic/libqtuiotouchplugin.so
    ${PREBUILT_ROOT}/plugins/generic/libqevdevtouchplugin.so>lib64/qt/plugins/generic/libqevdevtouchplugin.so
    ${PREBUILT_ROOT}/plugins/bearer/libqgenericbearer.so>lib64/qt/plugins/bearer/libqgenericbearer.so
    ${PREBUILT_ROOT}/plugins/bearer/libqconnmanbearer.so>lib64/qt/plugins/bearer/libqconnmanbearer.so
    ${PREBUILT_ROOT}/plugins/bearer/libqnmbearer.so>lib64/qt/plugins/bearer/libqnmbearer.so
    ${PREBUILT_ROOT}/plugins/imageformats/libqsvg.so>lib64/qt/plugins/imageformats/libqsvg.so
    ${PREBUILT_ROOT}/plugins/imageformats/libqtga.so>lib64/qt/plugins/imageformats/libqtga.so
    ${PREBUILT_ROOT}/plugins/imageformats/libqdds.so>lib64/qt/plugins/imageformats/libqdds.so
    ${PREBUILT_ROOT}/plugins/imageformats/libqwebp.so>lib64/qt/plugins/imageformats/libqwebp.so
    ${PREBUILT_ROOT}/plugins/imageformats/libqico.so>lib64/qt/plugins/imageformats/libqico.so
    ${PREBUILT_ROOT}/plugins/imageformats/libqicns.so>lib64/qt/plugins/imageformats/libqicns.so
    ${PREBUILT_ROOT}/plugins/imageformats/libqgif.so>lib64/qt/plugins/imageformats/libqgif.so
    ${PREBUILT_ROOT}/plugins/imageformats/libqjpeg.so>lib64/qt/plugins/imageformats/libqjpeg.so
    ${PREBUILT_ROOT}/plugins/imageformats/libqtiff.so>lib64/qt/plugins/imageformats/libqtiff.so
    ${PREBUILT_ROOT}/plugins/imageformats/libqwbmp.so>lib64/qt/plugins/imageformats/libqwbmp.so
    ${PREBUILT_ROOT}/plugins/platforms/libqoffscreen.so>lib64/qt/plugins/platforms/libqoffscreen.so
    ${PREBUILT_ROOT}/plugins/platforms/libqxcb.so>lib64/qt/plugins/platforms/libqxcb.so
    ${PREBUILT_ROOT}/plugins/platforms/libqminimal.so>lib64/qt/plugins/platforms/libqminimal.so
    ${PREBUILT_ROOT}/plugins/platforms/libqlinuxfb.so>lib64/qt/plugins/platforms/libqlinuxfb.so
    )
endif()

set(
  PACKAGE_EXPORT
  "QT5_INCLUDE_DIR;QT5_INCLUDE_DIRS;QT5_LIBRARIES;QT5_FOUND;QT5_CFLAGS;CMAKE_AUTOMOC;CMAKE_AUTOUIC;CMAKE_AUTORCC;QT_MOC_EXECUTABLE;QT_UIC_EXECUTABLE;QT_RCC_EXECUTABLE;QT_VERSION_MAJOR;QT5_SHARED_DEPENDENCIES;QT5_SHARED_PROPERTIES"
  )

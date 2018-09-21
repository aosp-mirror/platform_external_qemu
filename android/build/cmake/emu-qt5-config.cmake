# QT Settings ##
get_filename_component(
  PREBUILT_ROOT
  "${LOCAL_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/${LOCAL_TARGET_TAG}"
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
set(QT5_LIBRARIES
    -lQt5Widgets
    -lQt5Gui
    -lQt5Core
    -lQt5Svg
    -L${PREBUILT_ROOT}/bin)
set(QT5_CFLAGS "-DQT5_STATICLIB")
set(QT5_FOUND TRUE)

if(APPLE OR LOCAL_TARGET_TAG STREQUAL "darwin-x86_64")
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
elseif(LOCAL_TARGET_TAG MATCHES "windows.*")
  # On Windows, linking to mingw32 is required. The library is provided by the
  # toolchain, and depends on a main() function provided by qtmain which itself
  # depends on qMain(). These must appear in LDFLAGS and not LDLIBS since
  # qMain() is provided by object/libraries that appear after these in the link
  # command-line.
  set(QT5_LIBRARIES ${QT5_LIBRARIES} -lmingw32 ${PREBUILT_ROOT}/lib/libqtmain.a)
elseif(LOCAL_TARGET_TAG
       STREQUAL
       "linux-x86_64"
       OR
       LOCAL_TARGET_TAG
       STREQUAL
       "darwin-x86_64")
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
  "QT5_INCLUDE_DIR;QT5_INCLUDE_DIRS;QT5_LIBRARIES;QT5_FOUND;QT5_CFLAGS;CMAKE_AUTOMOC;CMAKE_AUTOUIC;CMAKE_AUTORCC;QT_MOC_EXECUTABLE;QT_UIC_EXECUTABLE;QT_RCC_EXECUTABLE;QT_MAJOR_VERSION;QT5_SHARED_DEPENDENCIES"
  )

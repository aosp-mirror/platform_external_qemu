## QT Settings ##
get_filename_component(PREBUILT_ROOT "${LOCAL_QEMU2_TOP_DIR}/../../prebuilts/android-emulator-build/qt/${LOCAL_TARGET_TAG}" ABSOLUTE)

if(NOT TARGET Qt5::moc)
    set(QT_VERSION_MAJOR 5)
    set(CMAKE_AUTOMOC TRUE)
    set(CMAKE_AUTOUIC TRUE)
    set(CMAKE_AUTORCC TRUE)
    set(QT_MOC_EXECUTABLE ${PREBUILT_ROOT}/bin/moc)
    set(QT_UIC_EXECUTABLE ${PREBUILT_ROOT}/bin/uic)
    set(QT_RCC_EXECUTABLE ${PREBUILT_ROOT}/bin/rcc)
    add_executable(Qt5::moc IMPORTED GLOBAL)
    set_property(TARGET Qt5::moc PROPERTY IMPORTED_LOCATION  ${QT_MOC_EXECUTABLE})
    add_executable(Qt5::uic IMPORTED GLOBAL)
    set_property(TARGET Qt5::uic PROPERTY IMPORTED_LOCATION  ${QT_UIC_EXECUTABLE})
    add_executable(Qt5::rcc IMPORTED GLOBAL)
    set_property(TARGET Qt5::rcc PROPERTY IMPORTED_LOCATION  ${QT_RCC_EXECUTABLE})
endif()

set(QT5_INCLUDE_DIRS ${PREBUILT_ROOT}/include
${PREBUILT_ROOT}/include.system
${PREBUILT_ROOT}/include.system/QtCore
${PREBUILT_ROOT}/include/QtCore
${PREBUILT_ROOT}/include/QtGui
${PREBUILT_ROOT}/include/QtSvg
${PREBUILT_ROOT}/include/QtWidgets
    )

set(QT5_INCLUDE_DIR ${QT5_INCLUDE_DIRS})
set(QT5_LIBRARIES -lQt5Widgets -lQt5Gui -lQt5Core -lQt5Svg -L${PREBUILT_ROOT}/bin ${PREBUILT_ROOT}/lib/libqtmain.a)
set(QT5_CFLAGS "-DQT5_STATICLIB")
set(QT5_FOUND TRUE)

if(APPLE OR LOCAL_TARGET_TAG STREQUAL "darwin-x86_64")
elseif (LOCAL_TARGET_TAG MATCHES "windows.*")
    # On Windows, linking to mingw32 is required. The library is provided
    # by the toolchain, and depends on a main() function provided by qtmain
    # which itself depends on qMain(). These must appear in LDFLAGS and
    # not LDLIBS since qMain() is provided by object/libraries that
    # appear after these in the link command-line.
    set(QT5_LIBRARIES ${QT5_LIBRARIES -lmingw32)
elseif (LINUX OR LOCAL_TARGET_TAG STREQUAL "linux-x86_64")
endif()

set(PACKAGE_EXPORT "QT5_INCLUDE_DIR;QT5_INCLUDE_DIRS;QT5_LIBRARIES;QT5_FOUND;QT5_CFLAGS;CMAKE_AUTOMOC;CMAKE_AUTOUIC;CMAKE_AUTORCC;QT_MOC_EXECUTABLE;QT_UIC_EXECUTABLE;QT_RCC_EXECUTABLE;QT_MAJOR_VERSION")

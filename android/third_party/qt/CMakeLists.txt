# The subdirectory under QT_SOURCE_DIR on which version of Qt we are using.
# Change this when upgrading to newer Qt versions.
set(QT_VERSION 5.12.1)
set(QT_SOURCE_DIR ${ANDROID_QEMU2_TOP_DIR}/../qt/${QT_VERSION})
# ============================================================================
# Qt
# ============================================================================

include(ExternalProject)
# TODO(joshuaduong): Builds so far completed:
# 1) linux (no-webengine) (WIP)
# 2) linux (webengine) (NO)
# 3) mingw (NO)
# 4) darwin (NO)
# 5) MSVC (NO)
set(QT_MODULES_NO_WEBENGINE
    qtbase
    qtsvg
    qtimageformats)
set(QT_MODULES ${QT_MODULES_NO_WEBENGINE})
set(QT_BUILD_MODULES "")
set(QT_INSTALL_MODULES "")
foreach(module ${QT_MODULES})
  list(APPEND QT_BUILD_MODULES "module-${module}")
  list(APPEND QT_INSTALL_MODULES "module-${module}-install_subtargets")
endforeach()
ExternalProject_add(Qt
                    SOURCE_DIR ${QT_SOURCE_DIR}
		    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/qt
		    INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/qt/install
		    CONFIGURE_COMMAND ${QT_SOURCE_DIR}/configure
			              -prefix ${CMAKE_CURRENT_BINARY_DIR}/qt/install
                                      -opensource
				      -confirm-license
				      -force-debug-info
				      -release
				      -no-rpath
				      -shared
				      -nomake examples
				      -nomake tests
				      -no-strip
				      -v
				      -qt-xcb
				      -no-use-gold-linker
				      -platform linux-clang
				      BUILD_COMMAND make -j48 V=1 ${QT_BUILD_MODULES})


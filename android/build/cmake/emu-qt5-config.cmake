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
  set(shared_lib_suffix "*.dylib")
  set(src_lib_dir "${PREBUILT_ROOT}/lib")
  set(dst_lib_dir "/lib64/qt/lib")
  set(src_plugins_dir "${PREBUILT_ROOT}/plugins")
  set(dst_plugins_dir "/lib64/qt/plugins")
  set(QT5_LIBRARIES -L${src_lib_dir} ${QT5_LIBRARIES})
  # Note: this will only set the property for install targets, not during build.
  set(QT5_SHARED_PROPERTIES "INSTALL_RPATH>=@loader_path/lib64/qt/lib;BUILD_WITH_INSTALL_RPATH=ON;LINK_FLAGS>=-framework Cocoa;")

elseif(ANDROID_TARGET_TAG STREQUAL "linux-x86_64")
  # .so* to pick up the symbolic links as well
  set(shared_lib_suffix "*.so*")
  set(src_lib_dir "${PREBUILT_ROOT}/lib")
  set(dst_lib_dir "/lib64/qt/lib")
  set(src_plugins_dir "${PREBUILT_ROOT}/plugins")
  set(dst_plugins_dir "/lib64/qt/plugins")
  set(QT5_LIBRARIES -L${src_lib_dir} ${QT5_LIBRARIES})

elseif(ANDROID_TARGET_OS_FLAVOR STREQUAL "windows")
  set(shared_lib_suffix "*.dll")
  # the dlls are in the bin/ directory for windows, but we copy it to the target lib directory.
  set(src_lib_dir "${PREBUILT_ROOT}/bin")
  set(src_plugins_dir "${PREBUILT_ROOT}/plugins")

  if(ANDROID_TARGET_TAG STREQUAL "windows-x86")
    set(dst_lib_dir "/lib/qt/lib")
    set(dst_plugins_dir "/lib/qt/plugins")
  else()
    set(dst_lib_dir "/lib64/qt/lib64")
    set(dst_plugins_dir "/lib64/qt/plugins")
  endif()

  if(ANDROID_TARGET_OS STREQUAL "windows")
    set(QT5_LIBRARIES
        -L${src_lib_dir}
        ${QT5_LIBRARIES})
    # On Windows mingw build, linking to mingw32 is required. The library is provided by the
    # toolchain, and depends on a main() function provided by qtmain which itself
    # depends on qMain(). These must appear in LDFLAGS and not LDLIBS since
    # qMain() is provided by object/libraries that appear after these in the link
    # command-line.
    set(
      QT5_LIBRARIES
      ${QT5_LIBRARIES}
      -lmingw32
      ${PREBUILT_ROOT}/lib/libqtmain.a)
  elseif(ANDROID_TARGET_OS STREQUAL "windows_msvc")
    # Clang/VS doesn't support linking directly to dlls. We linking to the import libraries
    # instead (.lib).
    set(QT5_LIBRARIES
        -L${src_lib_dir}/../lib
        ${QT5_LIBRARIES})
  else()
    message(FATAL_ERROR "Unknown flavor of Windows [${ANDROID_TARGET_TAG}]. \
            Need to add support for this configuration.")
  endif()

else()
  message(FATAL_ERROR "Please add support for this build target [${ANDROID_TARGET_TAG}].")
endif()

# Copy the dlls from the lib and plugins directory
foreach (dllpath lib plugins)
  file(GLOB_RECURSE qtdlls
       LIST_DIRECTORIES false
       RELATIVE ${src_${dllpath}_dir}
       ${src_${dllpath}_dir}/${shared_lib_suffix})
  foreach (qtdll ${qtdlls})
    set(QT5_SHARED_DEPENDENCIES
        ${QT5_SHARED_DEPENDENCIES}
	${src_${dllpath}_dir}/${qtdll}>${dst_${dllpath}_dir}/${qtdll})
  endforeach (qtdll)
endforeach (dllpath)

set(
  PACKAGE_EXPORT
  "QT5_INCLUDE_DIR;QT5_INCLUDE_DIRS;QT5_LIBRARIES;QT5_FOUND;QT5_CFLAGS;CMAKE_AUTOMOC;CMAKE_AUTOUIC;CMAKE_AUTORCC;QT_MOC_EXECUTABLE;QT_UIC_EXECUTABLE;QT_RCC_EXECUTABLE;QT_VERSION_MAJOR;QT5_SHARED_DEPENDENCIES;QT5_SHARED_PROPERTIES"
  )

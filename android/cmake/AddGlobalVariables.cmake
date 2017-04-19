if(__add_global_variables)
  return()
endif()
set(__add_global_variables INCLUDED)

function(find_root CURR)
  if (NOT EXISTS ${CURR})
    message(FATAL_ERROR "Unable to find repo root!")
  endif()
  if(EXISTS ${CURR}/external AND EXISTS ${CURR}/frameworks AND
      EXISTS ${CURR}/prebuilts AND EXISTS ${CURR}/sdk AND
      EXISTS ${CURR}/tools)
    SET(REPO_ROOT "${CURR}" PARENT_SCOPE)
  else()
    find_root(${CURR}/..)
    SET(REPO_ROOT "${REPO_ROOT}" PARENT_SCOPE)
  endif()
endfunction(find_root)

function(add_global_variables)
  find_root(${CMAKE_CURRENT_SOURCE_DIR})

  if(APPLE)
    set(BUILD_HOST_TAG64 darwin-x86_64)
    set(BUILD_HOST_TAG64 darwin-x86_64 PARENT_SCOPE)
    set(BUILD_HOST_EXT dylib PARENT_SCOPE)
    FIND_LIBRARY(CARBON_LIBRARY Carbon)
    FIND_LIBRARY(APP_SERVICES_LIBRARY ApplicationServices)
    MARK_AS_ADVANCED (CARBON_LIBRARY APP_SERVICES_LIBRARY)
    SET(EXTRA_LIBS ${CARBON_LIBRARY} ${APP_SERVICES_LIBRARY} PARENT_SCOPE)
    add_definitions(-D_DARWIN_C_SOURCE)
  elseif(UNIX)
    set(BUILD_HOST_TAG64 linux-x86_64)
    set(BUILD_HOST_TAG64 linux-x86_64 PARENT_SCOPE)
    set(BUILD_HOST_EXT so PARENT_SCOPE)
  elseif(WIN32)
    set(BUILD_HOST_TAG64 windows-x86_64)
    set(BUILD_HOST_TAG64 windows-x86_64 PARENT_SCOPE)
    # set(BUILD_HOST_TAG64 windows-x86) TODO FIX
  else()
    message(FATAL_ERROR "Unsupported OS")
  endif()

  set(PREBUILTS ${REPO_ROOT}/prebuilts)
  set(EXTERNAL ${REPO_ROOT}/external)
  set(QEMU ${REPO_ROOT}/external/qemu)
  set(QEMU_DEPS ${ANDROID_EMULATOR_BUILD}/qemu-android-deps/${BUILD_HOST_TAG64})
  set(ANDROID_EMULATOR_BUILD ${PREBUILTS}/android-emulator-build)
  set(ANDROID_EMULATOR_BUILD ${PREBUILTS}/android-emulator-build PARENT_SCOPE)

  set(ANDROID       ${QEMU}/android PARENT_SCOPE)
  set(BENCHMARK     ${EXTERNAL}/google-benchmark PARENT_SCOPE)
  set(ANDROID_EMU   ${QEMU}/android/android-emu PARENT_SCOPE)
  set(ANDROID_EMUGL ${QEMU}/android/android-emugl PARENT_SCOPE)


  set(Protobuf_LIBRARIES         ${ANDROID_EMULATOR_BUILD}/protobuf/${BUILD_HOST_TAG64}/lib/libprotobuf.a PARENT_SCOPE)
  set(Protobuf_INCLUDE_DIR ${ANDROID_EMULATOR_BUILD}/protobuf/${BUILD_HOST_TAG64}/include PARENT_SCOPE)
  set(Protobuf_PROTOC_EXECUTABLE ${ANDROID_EMULATOR_BUILD}/protobuf/${BUILD_HOST_TAG64}/bin/protoc PARENT_SCOPE)
  set(CONFIG_HOST_INCLUDE_DIRS ${QEMU}/android-qemu2-glue/config/${BUILD_HOST_TAG64} PARENT_SCOPE)

  # message("Find your bufs ${Protobuf_INCLUDE_DIR}, aa: ${aa} bb: ${bb}")

  set(GOOGLE_BENCHMARK_INCLUDE_DIRS
    ${EXTERNAL}/google-benchmark/include
    PARENT_SCOPE
    )

  set(ANGLE_INCLUDE_DIRS
    ${ANDROID_EMULATOR_BUILD}/common/ANGLE/${BUILD_HOST_TAG64}/include
    PARENT_SCOPE
    )

  set(BREAKPAD_INCLUDE_DIRS
    ${ANDROID_EMULATOR_BUILD}/common/breakpad/${BUILD_HOST_TAG64}/include/breakpad
    PARENT_SCOPE
    )

  set(CURL_INCLUDE_DIRS
    ${ANDROID_EMULATOR_BUILD}/curl/${BUILD_HOST_TAG64}/include
    PARENT_SCOPE
    )

  set(XML_INCLUDE_DIRS
    ${ANDROID_EMULATOR_BUILD}/common/libxml2/${BUILD_HOST_TAG64}/include
    PARENT_SCOPE
    )
  set(QT5_INCLUDE_DIRS
    ${ANDROID_EMULATOR_BUILD}/qt/common/include
    ${ANDROID_EMULATOR_BUILD}/qt/common/include/QtCore
    ${ANDROID_EMULATOR_BUILD}/qt/common/include/QtWidgets
    ${ANDROID_EMULATOR_BUILD}/qt/common/include/QtGui
    ${ANDROID_EMULATOR_BUILD}/qt/${BUILD_HOST_TAG64}/include.system
    PARENT_SCOPE
    )


  set(ANGLE_LIBRARIES
    ${ANDROID_EMULATOR_BUILD}/common/ANGLE/${BUILD_HOST_TAG64}/lib/libangle_common.a
    ${ANDROID_EMULATOR_BUILD}/common/ANGLE/${BUILD_HOST_TAG64}/lib/libpreprocessor.a
    ${ANDROID_EMULATOR_BUILD}/common/ANGLE/${BUILD_HOST_TAG64}/lib/libtranslator_lib.a
    ${ANDROID_EMULATOR_BUILD}/common/ANGLE/${BUILD_HOST_TAG64}/lib/libtranslator_static.a
    PARENT_SCOPE
    )


endfunction(add_global_variables)

function(add_qt_support TARGET)
  find_root(${CMAKE_CURRENT_SOURCE_DIR})
  set(PREBUILTS ${REPO_ROOT}/prebuilts)
  message("Prebuilts: ${PREBUILTS}")
  set(ANDROID_EMULATOR_BUILD ${PREBUILTS}/android-emulator-build)

  set(QT_VERSION_MAJOR 5 PARENT_SCOPE)
  set(CMAKE_AUTOMOC TRUE PARENT_SCOPE)
  set(CMAKE_AUTOUIC TRUE PARENT_SCOPE)

  set(QT_MOC_EXECUTABLE ${ANDROID_EMULATOR_BUILD}/qt/${BUILD_HOST_TAG64}/bin/moc)
  set(QT_UIC_EXECUTABLE ${ANDROID_EMULATOR_BUILD}/qt/${BUILD_HOST_TAG64}/bin/uic)

  add_executable(Qt5::moc IMPORTED GLOBAL)
  add_executable(Qt5::uic IMPORTED GLOBAL)
  set_property(TARGET Qt5::moc PROPERTY IMPORTED_LOCATION ${QT_MOC_EXECUTABLE})
  set_property(TARGET Qt5::uic PROPERTY IMPORTED_LOCATION ${QT_UIC_EXECUTABLE})


  if(APPLE)
    # We need to fix up this error when running uic:
    # dyld: Library not loaded: /tmp/ssh-build-qt/build/install-darwin-x86_64/lib/libQt5Core.5.dylib
    # We hack around it by symlinking this to the actual dylib
    file(MAKE_DIRECTORY  /tmp/ssh-build-qt/build/install-darwin-x86_64/lib/)
    ADD_CUSTOM_TARGET(fix_uic ALL
      COMMAND ${CMAKE_COMMAND} -E create_symlink
      ${ANDROID_EMULATOR_BUILD}/qt/darwin-x86_64/lib/libQt5Core.5.dylib
      /tmp/ssh-build-qt/build/install-darwin-x86_64/lib/libQt5Core.5.dylib)
    add_dependencies(${TARGET} fix_uic)
  endif()

  set(QT5_LIBS
    ${ANDROID_EMULATOR_BUILD}/qt/${BUILD_HOST_TAG64}/lib/libQt5Widgets.${BUILD_HOST_EXT}
    ${ANDROID_EMULATOR_BUILD}/qt/${BUILD_HOST_TAG64}/lib/libQt5Core.${BUILD_HOST_EXT}
    ${ANDROID_EMULATOR_BUILD}/qt/${BUILD_HOST_TAG64}/lib/libQt5Gui.${BUILD_HOST_EXT})


  target_link_libraries(${TARGET} ${QT5_LIBS})
endfunction(add_qt_support)

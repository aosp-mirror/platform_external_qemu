set(WINDOWS_ICON ../images/emulator_icon.rc)

add_executable(emulator main-emulator.cpp ${WINDOWS_ICON})
target_link_libraries(emulator android-emu)
target_compile_definitions (
  android-emu
  PRIVATE
  -DCONFIG_QT
  -DANDROID_SDK_TOOLS_REVISION=${OPTION_SDK_TOOLS_REVISION}
  -DANDROID_SDK_TOOLS_BUILD_NUMBER=${OPTION_SDK_TOOLS_BUILD_NUMBER}
  )
target_prebuilt_dependency(emulator "${RUNTIME_OS_DEPENDENCIES}" "${RUNTIME_OS_PROPERTIES}")

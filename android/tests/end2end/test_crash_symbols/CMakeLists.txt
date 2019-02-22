# This tries to guess if we will be able to run the symbol end-to-end test
set(ANDROID_HOME $ENV{ANDROID_HOME})
if(NOT ANDROID_HOME)
  get_filename_component(ANDROID_HOME "$ENV{HOME}/.android" ABSOLUTE)
endif()

if(ANDROID_HOME)
  file(GLOB AVDS "${ANDROID_HOME}/avd/*.ini")
endif()

if(AVDS AND ANDROID_EXTRACT_SYMBOLS AND (NOT OPTION_CRASHUPLOAD STREQUAL "NONE"))
  # Looks like we have some avds..
  add_test(NAME proper_symbols_test
           COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/launch_from_cmake.py
                   --build ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
  set_tests_properties(proper_symbols_test PROPERTIES DEPENDS emulator)
  # add_dependencies(proper_symbols_test emulator)

else()
  message("No symbol test scheduled.")
endif()

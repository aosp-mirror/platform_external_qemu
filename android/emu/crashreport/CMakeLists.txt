prebuilt(QT5)
android_add_library(
  TARGET android-emu-crash-initializer
  # SHARED # Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/crashreport/CrashSytemInit.cpp
  DEPS android-emu-base
       android-emu-base-headers
       android-emu-crashreport
       android-emu-files
       android-emu-utils
       crashpad_client
       protobuf::libprotobuf)
target_link_libraries(android-emu-crash-initializer
                      PUBLIC android-emu-base-headers)
target_include_directories(android-emu-crash-initializer PRIVATE src
                           PUBLIC include)
target_compile_options(android-emu-crash-initializer
                       PRIVATE -Wno-extern-c-compat PUBLIC -Wno-macro-redefined)

android_add_library(
  TARGET android-emu-crashreport
  # SHARED # Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/crashreport/CrashReporter.cpp
      android/crashreport/detectors/CrashDetectors.cpp
      android/crashreport/HangDetector.cpp
  DEPS android-emu-agents
       android-emu-avd
       android-emu-base
       android-emu-cmdline
       android-emu-files
       android-emu-utils
       android-hw-config
       protobuf::libprotobuf)
target_include_directories(android-emu-crashreport PRIVATE src PUBLIC include)
target_link_libraries(android-emu-crashreport PUBLIC crashpad_client
                                                     android-emu-base-headers)
target_compile_options(android-emu-crashreport PRIVATE -Wno-extern-c-compat
                       PUBLIC -Wno-macro-redefined)

# This file defines emulator crash service
android_add_library(
  TARGET android-emu-crashreport-consent-ui
  # SHARED # Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/crashreport/ui/ConfirmDialog.cpp
      android/crashreport/ui/CrashConsentUi.cpp
      android/crashreport/ui/UserSuggestions.cpp
  DEPS android-emu
       android-emu-agents
       android-emu-base
       android-emu-base-headers
       android-emu-cmdline
       android-emu-utils
       breakpad_processor
       crashpad_client
       emulator-libui
       ui::widgets
       Qt${QT_VERSION_MAJOR}::Widgets)
target_compile_definitions(android-emu-crashreport-consent-ui
                           PRIVATE AEMU_CRASH_EXPORTS PUBLIC AEMU_CRASH_SHARED)
target_include_directories(android-emu-crashreport-consent-ui PRIVATE src
                           PUBLIC include)
target_compile_options(android-emu-crashreport-consent-ui
                       PRIVATE -Wno-extern-c-compat)

android_add_library(
  TARGET android-emu-crashreport-consent-no-ui
  # SHARED # Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/crashreport/CrashConsentNoUi.cpp
  DEPS android-emu-agents android-emu-base android-emu-base-headers
       android-emu-cmdline android-emu-utils crashpad_client)
target_compile_definitions(android-emu-crashreport-consent-no-ui
                           PRIVATE AEMU_CRASH_EXPORTS PUBLIC AEMU_CRASH_SHARED)
target_include_directories(android-emu-crashreport-consent-no-ui PRIVATE src
                           PUBLIC include)
target_compile_options(android-emu-crashreport-consent-no-ui
                       PRIVATE -Wno-extern-c-compat)

# This one can be used for unit tests.
android_add_library(
  TARGET android-emu-crashreport-consent-never
  # SHARED # Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/crashreport/CrashConsentNever.cpp
  DEPS crashpad_client)
target_compile_definitions(android-emu-crashreport-consent-never
                           PRIVATE AEMU_CRASH_EXPORTS PUBLIC AEMU_CRASH_SHARED)
target_include_directories(android-emu-crashreport-consent-never PRIVATE src
                           PUBLIC include)

# This one can be used for unit tests.
android_add_library(
  TARGET android-emu-crashreport-consent-always
  # SHARED # Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/crashreport/CrashConsentAlways.cpp
  DEPS crashpad_client)
target_compile_definitions(android-emu-crashreport-consent-always
                           PRIVATE AEMU_CRASH_EXPORTS PUBLIC AEMU_CRASH_SHARED)
target_include_directories(android-emu-crashreport-consent-always PRIVATE src
                           PUBLIC include)

android_add_executable(
  TARGET crashreport INSTALL .
  LICENSE Apache-2.0 SOURCE_DIR
  src
  SRC android/crashreport/CrashUploader.cpp
  DEPS android-emu-base-headers android-emu-crashreport
       android-emu-crashreport-consent-always breakpad_common
       breakpad_processor crashpad_handler_handler)
add_dependencies(crashreport crashpad_handler)

android_add_executable(
  TARGET crash-me
  NODISTRIBUTE SOURCE_DIR test
  SRC android/crashreport/testing/main-test-crasher.cpp
  DEPS android-emu android-emu-crash-initializer android-emu-crashreport
       android-emu-crashreport-consent-never)

android_upload_symbols(TARGET crash-me DIRECTORY ${ANDROID_SYMBOL_DIR})

android_add_test(
  TARGET android-emu-crashreport-testing
  SRC android/crashreport//CrashDump_unittest.cpp
      android/crashreport/detectors/CrashDetectors_unittest.cpp
  SOURCE_DIR test
  DEPS android-emu-agents
       android-emu-base
       android-emu-base-headers
       android-emu-base-process
       android-emu-crashreport
       android-emu-test-launcher
       android-emu-utils
       breakpad_processor
       crashpad_client)

target_compile_options(android-emu-crashreport-testing
                       PRIVATE -Wno-extern-c-compat)
add_dependencies(android-emu-crashreport-testing crash-me crashpad_handler
                 crashreport)

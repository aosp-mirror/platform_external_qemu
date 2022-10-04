# A library that contains harware related things
prebuilt(LIBXML2)
android_add_library(
  TARGET android-emu-telephony
  # SHARED # Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/telephony/debug.c
      android/telephony/gsm.c
      android/telephony/MeterService.cpp
      android/telephony/modem.c
      android/telephony/modem_driver.c
      android/telephony/phone_number.cpp
      android/telephony/remote_call.c
      android/telephony/sim_card.c
      android/telephony/SimAccessRules.cpp
      android/telephony/sms.c
      android/telephony/sysdeps.c
      android/telephony/TagLengthValue.cpp
  DEPS android-emu-adb-interface
       android-emu-agents
       android-emu-avd
       android-emu-base
       android-emu-base-headers
       android-emu-cmdline
       android-emu-feature
       android-emu-files
       android-emu-hardware
       android-emu-protos
       android-emu-proxy
       android-emu-telnet-console-auth
       android-emu-utils
       android-hw-config
       LibXml2::LibXml2
       protobuf::libprotobuf)
target_include_directories(android-emu-telephony PRIVATE src PUBLIC include)
target_compile_options(android-emu-telephony PRIVATE -Wno-extern-c-compat)

android_add_test(
  TARGET android-emu-telephony_unittests
  SOURCE_DIR test
  SRC android/telephony/gsm_unittest.cpp
      android/telephony/modem_unittest.cpp
      android/telephony/SimAccessRules_unittest.cpp
      android/telephony/sms_unittest.cpp
      android/telephony/TagLengthValue_unittest.cpp
  DEPS android-emu-agents
       android-emu-base
       android-emu-base-headers
       android-emu-cmdline
       android-emu-hardware
       android-emu-protos
       android-emu-telephony
       android-emu-test-launcher
       android-emu-utils
       gmock
       protobuf::libprotobuf)
target_compile_options(android-emu-telephony_unittests PRIVATE -Wno-extern-c-compat)
target_include_directories(android-emu-telephony_unittests PRIVATE src test)

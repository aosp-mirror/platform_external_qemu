# A library that contains harware related things
android_add_library(
  TARGET android-emu-wear
  # SHARED # Used to test if you can compile this stand alone.
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/wear-agent/android_wear_agent.cpp
      android/wear-agent/PairUpWearPhone.cpp android/wear-agent/WearAgent.cpp
  DEPS android-emu-agents android-emu-base android-emu-base-headers android-emu-hardware
       android-emu-utils)
target_include_directories(android-emu-wear PRIVATE src PUBLIC include)

android_add_test(
  TARGET android-emu-wear_unittests
  SOURCE_DIR test
  SRC android/wear-agent/PairUpWearPhone_unittest.cpp
      android/wear-agent/testing/WearAgentTestUtils.cpp
      android/wear-agent/WearAgent_unittest.cpp
  DEPS android-emu-hardware android-emu-base-headers android-emu-test-launcher android-emu-wear
       android-emu-crashreport-consent-never)
  
target_include_directories(android-emu-wear_unittests PRIVATE src test)

android_add_library(
  TARGET android-grpc-security
  # SHARED # Testing only to make sure all dependencies are defined.
  LICENSE Apache-2.0
  SOURCE_DIR src
  SRC android/emulation/control/secure/AllowList.cpp
      android/emulation/control/secure/BasicTokenAuth.cpp
      android/emulation/control/secure/JwkDirectoryObserver.cpp
      android/emulation/control/secure/JwkKeyLoader.cpp
      android/emulation/control/secure/JwtTokenAuth.cpp
  DEPS android-emu-base tink::jwt::jwk_set_converter
       tink::jwt::jwt_public_key_verify tink::jwt::jwt_signature_config
       tink::jwt::jwt_validator tink::jwt::verified_jwt)

target_link_libraries(
  android-grpc-security
  PUBLIC android-emu-base-headers tink::config::tink_config
         nlohmann_json::nlohmann_json grpc++)
target_include_directories(android-grpc-security PRIVATE src PUBLIC include)

install(
  FILES
    ${CMAKE_CURRENT_LIST_DIR}/src/android/emulation/control/secure/emulator_access.json
  DESTINATION lib)

add_custom_command(
  TARGET android-grpc-security
  PRE_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_LIST_DIR}/src/android/emulation/control/secure/emulator_access.json
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib/emulator_access.json)

android_add_test(
  TARGET android-grpc-security_unittests
  SOURCE_DIR test
  SRC android/emulation/control/secure/AllowList_unittest.cpp
      android/emulation/control/secure/JwkDirectoryObserver_unittest.cpp
      android/emulation/control/secure/JwtTokenAuth_unittest.cpp
      android/emulation/control/secure/JwkKeyLoader_unittest.cpp
  DEPS android-emu-base android-grpc-security gmock_main grpc++
       tink::jwt::jwt_key_templates tink::jwt::jwt_signature_config)

add_custom_command(
  TARGET android-grpc-security_unittests
  PRE_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_LIST_DIR}/test/android/emulation/control/secure/test_allow_list.json
    $<TARGET_FILE_DIR:android-grpc-security_unittests>/test_allow_list.json)

# Add the benchmark
android_add_executable(
  TARGET android-grpc-security_perf NODISTRIBUTE SOURCE_DIR test
  SRC android/emulation/control/secure/AllowListPerf.cpp
  DEPS android-emu-base android-grpc-security emulator-gbench)

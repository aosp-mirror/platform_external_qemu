# A set of sample apps.

android_add_executable(TARGET nimble_advertiser NODISTRIBUTE
                       SRC advertiser/src/main.c)
target_link_libraries(nimble_advertiser PRIVATE android-emu-base-headers nimble_bluetooth)
target_compile_options(nimble_advertiser
                       PRIVATE -Wno-implicit-function-declaration)

android_add_executable(TARGET nimble_blecsc NODISTRIBUTE
                       SRC blecsc/src/gatt_svr.c blecsc/src/main.c)
target_link_libraries(nimble_blecsc PRIVATE android-emu-base-headers nimble_bluetooth)
target_compile_options(
  nimble_blecsc
  PRIVATE -Wno-implicit-function-declaration
          -Wno-tentative-definition-incomplete-type -Wno-visibility)

# BLE Heart Rate peripheral app.
android_add_executable(TARGET nimble_blehr NODISTRIBUTE SRC blehr/src/gatt_svr.c
                                                            blehr/src/main.c)
target_link_libraries(nimble_blehr PRIVATE android-emu-base-headers nimble_bluetooth)
target_compile_options(nimble_blehr PRIVATE -Wno-implicit-function-declaration)

if(FALSE)
  # Disabled due to missing dependencies
  android_add_executable(
    TARGET nimble_bleprph NODISTRIBUTE
    SRC bleprph/src/gatt_svr.c bleprph/src/main.c bleprph/src/misc.c
        bleprph/src/phy.c)
  target_link_libraries(nimble_bleprph PRIVATE nimble_bluetooth)
  target_compile_options(nimble_bleprph
                         PRIVATE -Wno-implicit-function-declaration)

  android_add_executable(
    TARGET nimble_btshell
    NODISTRIBUTE
    SRC btshell/src/cmd.c
        btshell/src/cmd_gatt.c
        btshell/src/cmd_l2cap.c
        btshell/src/gatt_svr.c
        btshell/src/main.c
        btshell/src/misc.c
        btshell/src/parse.c)
  target_include_directories(nimble_btshell PRIVATE btshell/src)
  target_link_libraries(nimble_btshell PRIVATE nimble_bluetooth)
  target_compile_options(nimble_btshell
                         PRIVATE -Wno-implicit-function-declaration)

  android_add_executable(
    TARGET nimble_bttester
    NODISTRIBUTE
    SRC bttester/src/bttester.c
        bttester/src/gap.c
        bttester/src/gatt.c
        bttester/src/glue.c
        bttester/src/l2cap.c
        bttester/src/main.c
        bttester/src/mesh.c
        bttester/src/rtt_pipe.c
        bttester/src/uart_pipe.c)
  target_link_libraries(nimble_bttester nimble_bluetooth)
  target_compile_options(
    nimble_bttester PRIVATE -Wno-implicit-function-declaration
                            -Wno-tentative-definition-incomplete-type)

  # BLE_EXT_ADV is disabled. This enables extended advertising feature needs to
  # be enabled (crashes root canal)
  android_add_executable(TARGET nimble_ext_advertiser NODISTRIBUTE
                         SRC ext_advertiser/src/main.c)
  target_link_libraries(nimble_ext_advertiser nimble_bluetooth)
  target_include_directories(nimble_ext_advertiser PRIVATE ext_advertiser/src)
  target_compile_options(nimble_ext_advertiser
                         PRIVATE -Wno-implicit-function-declaration)

endif()

android_add_executable(TARGET nimble_central NODISTRIBUTE
                       SRC central/src/main.c)
target_link_libraries(nimble_central PRIVATE android-emu-base-headers nimble_bluetooth)
target_compile_options(nimble_central
                       PRIVATE -Wno-implicit-function-declaration)

android_add_executable(TARGET nimble_peripheral NODISTRIBUTE
                       SRC peripheral/src/main.c)
target_link_libraries(nimble_peripheral PRIVATE android-emu-base-headers nimble_bluetooth)
target_compile_options(nimble_peripheral
                       PRIVATE -Wno-implicit-function-declaration)

android_add_executable(TARGET nimble_scanner NODISTRIBUTE
                       SRC scanner/src/main.c)
target_link_libraries(nimble_scanner PRIVATE android-emu-base-headers nimble_bluetooth)
target_compile_options(nimble_scanner
                       PRIVATE -Wno-implicit-function-declaration)

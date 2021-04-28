# A set of sample apps.

add_executable(nimble_advertiser advertiser/src/main.c)
target_link_libraries(nimble_advertiser nimble_bluetooth)
target_compile_options(nimble_advertiser
                       PRIVATE -Wno-implicit-function-declaration)

add_executable(nimble_blecsc blecsc/src/gatt_svr.c blecsc/src/main.c)
target_link_libraries(nimble_blecsc nimble_bluetooth)
target_compile_options(
  nimble_blecsc
  PRIVATE -Wno-implicit-function-declaration
          -Wno-tentative-definition-incomplete-type -Wno-visibility)

# BLE Heart Rate peripheral app.
add_executable(nimble_blehr blehr/src/gatt_svr.c blehr/src/main.c)
target_link_libraries(nimble_blehr nimble_bluetooth)
target_compile_options(nimble_blehr PRIVATE -Wno-implicit-function-declaration)

if(FALSE)
  # Disabled due to missing dependencies
  add_executable(nimble_bleprph bleprph/src/gatt_svr.c bleprph/src/main.c
                                bleprph/src/misc.c bleprph/src/phy.c)
  target_link_libraries(nimble_bleprph nimble_bluetooth)
  target_compile_options(nimble_bleprph
                         PRIVATE -Wno-implicit-function-declaration)

  add_executable(
    nimble_btshell
    btshell/src/cmd.c
    btshell/src/cmd_gatt.c
    btshell/src/cmd_l2cap.c
    btshell/src/gatt_svr.c
    btshell/src/main.c
    btshell/src/misc.c
    btshell/src/parse.c)
  target_include_directories(nimble_btshell PRIVATE btshell/src)
  target_link_libraries(nimble_btshell nimble_bluetooth)
  target_compile_options(nimble_btshell
                         PRIVATE -Wno-implicit-function-declaration)

  add_executable(
    nimble_bttester
    bttester/src/bttester.c
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
  add_executable(nimble_ext_advertiser ext_advertiser/src/main.c)
  target_link_libraries(nimble_ext_advertiser nimble_bluetooth)
  target_include_directories(nimble_ext_advertiser PRIVATE ext_advertiser/src)
  target_compile_options(nimble_ext_advertiser
                         PRIVATE -Wno-implicit-function-declaration)

endif()

add_executable(nimble_central central/src/main.c)
target_link_libraries(nimble_central nimble_bluetooth)
target_compile_options(nimble_central
                       PRIVATE -Wno-implicit-function-declaration)

add_executable(nimble_peripheral peripheral/src/main.c)
target_link_libraries(nimble_peripheral nimble_bluetooth)
target_compile_options(nimble_peripheral
                       PRIVATE -Wno-implicit-function-declaration)

add_executable(nimble_scanner scanner/src/main.c)
target_link_libraries(nimble_scanner nimble_bluetooth)
target_compile_options(nimble_scanner
                       PRIVATE -Wno-implicit-function-declaration)

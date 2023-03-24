# This contains all the individual services that we can offer.
# If you add new services, make sure that you make the protobufs available to
# the python packages as well!

add_subdirectory(adb)
add_subdirectory(emulator-controller)
add_subdirectory(emulator-stats)
add_subdirectory(gnss)
add_subdirectory(incubating)
add_subdirectory(snapshot)
add_subdirectory(ui-controller)

if(BLUETOOTH_EMULATION)
  add_subdirectory(bluetooth)
endif()
add_subdirectory(waterfall)

add_subdirectory(nimble_bridge)

if(TARGET Rust::Rustc)
  add_subdirectory(rootcanal)
else()
  message(
    WARNING
      "Disabling rootcanal for bluetooth, no complete toolchain present (Windows/M1)")
endif()

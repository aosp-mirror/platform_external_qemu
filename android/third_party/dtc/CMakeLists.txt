set(LIBDTC_ROOT ${AOSP_ROOT}/external/dtc)

android_add_library(
  TARGET fdt
  LICENSE BSD-2-Clause
  SOURCE_DIR ${LIBDTC_ROOT}/libfdt
  SRC acpi.c
      fdt.c
      fdt_addresses.c
      fdt_check.c
      fdt_empty_tree.c
      fdt_overlay.c
      fdt_ro.c
      fdt_rw.c
      fdt_strerror.c
      fdt_sw.c
      fdt_wip.c)
target_include_directories(fdt PUBLIC ${LIBDTC_ROOT}/libfdt)

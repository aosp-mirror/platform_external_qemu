android_add_executable(
  TARGET qsn INSTALL .
  LICENSE Apache-2.0 SOURCE_DIR src
  SRC QueryEmulator.cpp
  DEPS android-emu android-emu-crashreport
       android-emu-crashreport-consent-never android-grpc-services-stack libqemu2-glue
       snapshot-service)

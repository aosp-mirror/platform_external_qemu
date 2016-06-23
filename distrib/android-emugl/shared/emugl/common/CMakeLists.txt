add_library(emugl_common STATIC
    crash_reporter.cpp
    feature_control.cpp
    logging.cpp
    shared_library.cpp
    stringparsing.cpp
    sockets.cpp
    )

target_include_directories(emugl_common
    PUBLIC
    ${REPO_QEMU}
    ${REPO_QEMU}/distrib/android-emugl/host/include
    ${REPO_QEMU}/distrib/android-emugl/shared
    )

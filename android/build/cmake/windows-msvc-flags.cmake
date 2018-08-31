set(triple x86_64-pc-win32)
set(CLANG_INTRINS_INCLUDE "/usr/local/google/home/joshuaduong/emu/master/prebuilts/android-emulator-build/msvc/clang-intrins/include")
set(MSVC_INCLUDE "/usr/local/google/home/joshuaduong/emu/master/prebuilts/android-emulator-build/msvc/msvc/include")
set(MSVC_LIBS "/usr/local/google/home/joshuaduong/emu/master/prebuilts/android-emulator-build/msvc/msvc/lib")
set(WINSDK_INCLUDE "/usr/local/google/home/joshuaduong/emu/master/prebuilts/android-emulator-build/msvc/win10sdk/include/10.0.16299.0")
set(WINSDK_LIBS "/usr/local/google/home/joshuaduong/emu/master/prebuilts/android-emulator-build/msvc/win10sdk/lib/10.0.16299.0")

set(COMPILE_FLAGS
    -D_CRT_SECURE_NO_WARNINGS
    -I${CLANG_INTRINS_INCLUDE}
    -I${MSVC_INCLUDE}
    -I${WINSDK_INCLUDE}/ucrt
    -I${WINSDK_INCLUDE}/shared
    -I${WINSDK_INCLUDE}/um
    -I${WINSDK_INCLUDE}/winrt
    -I${WINSDK_INCLUDE}/hypervisor
    -target ${triple}
    -fuse-ld=lld)
string(REPLACE ";" " " COMPILE_FLAGS "${COMPILE_FLAGS}")

set(CMAKE_C_FLAGS_INIT "${COMPILE_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE_INIT "")
set(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "")
set(CMAKE_C_FLAGS_MINSIZEREL_INIT "")
set(CMAKE_C_FLAGS_DEBUG_INIT "")

set(CMAKE_CXX_FLAGS_INIT "${COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "")
set(CMAKE_CXX_FLAGS_MINSIZEREL_INIT "")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "")

set(CMAKE_CL_NOLOGO_INIT "")

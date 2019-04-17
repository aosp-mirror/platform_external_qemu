cmake_minimum_required(VERSION 2.8.9)
project (goldfish-webrtc)

# Setup the various dependencies
if(UNIX AND NOT APPLE)

    set(CLANG_PREFIX "${WEBRTC_SRC_ROOT}")
    # Okay, so webrtc ships their own compiler, which will (likely) differ from what you have and
    # it will cause all sorts of havoc. So instead we will configure clang to use the webrtc config.
    # (Note, that clang-7 from android-emu works out of the box)
    set(DEB_RELEASE sid)
    set(CMAKE_CXX_COMPILER "${CLANG_PREFIX}/third_party/llvm-build/Release+Asserts/bin/clang++")

    # Make sure we can find the right C++11 headers and sysroot etc.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${CLANG_PREFIX}/buildtools/third_party/libc++/trunk/include")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${CLANG_PREFIX}/buildtools/third_party/libc++abi/trunk/include")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --sysroot=${CLANG_PREFIX}/build/linux/debian_${DEB_RELEASE}_amd64-sysroot/")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L ${CLANG_PREFIX}/build/linux/debian_${DEB_RELEASE}_amd64-sysroot/lib/x86_64-linux-gnu")

    # Configure the linker properly.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath-link=${CLANG_PREFIX}/build/linux/debian_${DEB_RELEASE}_amd64-sysroot/lib/x86_64-linux-gnu")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath-link=${CLANG_PREFIX}/build/linux/debian_${DEB_RELEASE}_amd64-sysroot/usr/lib/x86_64-linux-gnu")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath-link=. -Wl,--disable-new-dtags")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--fatal-warnings -fPIC -Wl,-z,noexecstack -Wl,-z,now")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-z,relro -Wl,-z,defs -Wl,--no-as-needed -lpthread -Wl,--as-needed")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fuse-ld=lld -Wl,--icf=all -m64 -Werror -Wl,--gdb-index -nostdlib++")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

    # Please no complaints about arguments.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument")

    # Makre sure we use the right kind of webrtc config
    add_definitions(-DWEBRTC_POSIX)
endif()
if (APPLE)
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")

    # Make sure we use the right kind of webrtc config
    add_definitions(-DWEBRTC_MAC)
    add_definitions(-DWEBRTC_POSIX)
    add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
endif()
if (WIN32)
    add_definitions(-DWEBRTC_WIN)
endif()

find_package (Threads)

#
# The set of source files, we include headers so you can use CLion
set(SOURCE_FILES
        src/emulator/base/TypeTraits.h
        src/emulator/base/memory/SharedMemory_posix.cpp
        src/emulator/webrtc/capture/VideoShareCapture.cc
        src/emulator/webrtc/capture/VideoShareFactory.cc
        src/emulator/webrtc/capture/VideoShareInfo.cc
        # src/emulator/webrtc/capture/video_capture_impl2.cc
        src/defaults.cc
        src/main.cc
        src/emulator/net/JsonProtocol.cc
        src/emulator/net/JsonProtocol.h
        src/emulator/net/SocketTransport.cc
        src/emulator/webrtc/Participant.cc
        src/emulator/webrtc/Switchboard.cc
)

add_executable(goldfish-webrtc-bridge ${SOURCE_FILES})
target_include_directories(goldfish-webrtc-bridge PRIVATE src
    ${WEBRTC_INSTALL_DIR}/include/src/third_party/abseil-cpp/
    ${WEBRTC_INSTALL_DIR}/include/src/third_party/libyuv/include
    ${WEBRTC_INSTALL_DIR}/include/src)

add_dependencies(goldfish-webrtc-bridge project-webrtc)


# The linker dependencies.
target_link_libraries(goldfish-webrtc-bridge ${WEBRTC_INSTALL_DIR}/lib/libwebrtc.a ${CMAKE_THREAD_LIBS_INIT})
if(UNIX AND NOT APPLE)
    target_link_libraries(goldfish-webrtc-bridge dl X11 rt)
endif()
if (APPLE)
    # Add the proper frameworks
    target_link_libraries(goldfish-webrtc-bridge
        /System/Library/Frameworks/AudioToolbox.framework;
        /System/Library/Frameworks/CoreAudio.framework;
        /System/Library/Frameworks/CoreFoundation.framework;
        /System/Library/Frameworks/CoreGraphics.framework;
        /System/Library/Frameworks/Foundation.framework;
        /System/Library/Frameworks/AVFoundation.framework;
        /System/Library/Frameworks/CoreVideo.framework;
        /System/Library/Frameworks/CoreMedia.framework)
endif()

install(TARGETS goldfish-webrtc-bridge RUNTIME DESTINATION bin)

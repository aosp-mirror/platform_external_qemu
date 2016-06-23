if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_compile_options(-DUSE_MINGW=1)
endif()

add_library(ext4_utils STATIC
    src/allocate.c
    src/contents.c
    src/crc16.c
    src/ext4_sb.c
    src/ext4_utils.c
    src/extent.c
    src/indirect.c
    src/make_ext4fs.c
    src/sha1.c
    src/uuid.c
    src/wipe.c
    )

target_include_directories(ext4_utils
    PRIVATE
    ./
    PUBLIC
    ./include
    ${REPO_PREBUILTS_AEB}/qemu-android-deps/${BUILD_TARGET_TAG}/include
    )

if(CMAKE_SYSTEM_NAME MATCHES "(Linux|Darwin)")
    set(EXT4_UTILS_PLATFORM_LIB
        selinux
        )
endif()

target_link_libraries(ext4_utils
    PUBLIC
    ${EXT4_UTILS_PLATFORM_LIB}
    PRIVATE
    sparse
    )

add_executable(emulator_make_ext4fs
    src/make_ext4fs_main.c
    )

target_link_libraries(emulator_make_ext4fs
    ext4_utils
    )


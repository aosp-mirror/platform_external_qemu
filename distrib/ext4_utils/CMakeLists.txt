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
    PRIVATE
    sparse
    ${EXT4_UTILS_PLATFORM_LIB}
    )

#[[

$(call start-emulator-program,emulator$(BUILD_TARGET_SUFFIX)_make_ext4fs)
LOCAL_SRC_FILES := src/make_ext4fs_main.c
LOCAL_C_INCLUDES := \
    $(LIBEXT4_UTILS_INCLUDES) \
    $(LIBSELINUX_INCLUDES)
LOCAL_CFLAGS := $(LIBEXT4_UTILS_CFLAGS)
LOCAL_STATIC_LIBRARIES := \
    emulator-libext4_utils \
    emulator-libsparse \
    emulator-libselinux \
    emulator-zlib
$(call end-emulator-program)

]]

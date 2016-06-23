add_library(sparse STATIC
        src/backed_block.c
        src/output_file.c
        src/sparse.c
        src/sparse_crc32.c
        src/sparse_err.c
        src/sparse_read.c
        )

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_compile_options(sparse
        PRIVATE
        -DUSE_MINGW=1
        )
endif()

target_include_directories(sparse
    PUBLIC
    ./include
    PRIVATE
    ${SPARSE_PLATFORM_INCLUDE}
    ${REPO_PREBUILTS_AEB}/common/e2fsprogs
    ${REPO_PREBUILTS_AEB}/qemu-android-deps/${BUILD_TARGET_TAG}/include
    ./src
    ./
    )

target_link_libraries(sparse
    PUBLIC
    -lz
    )

add_executable(emulator_img2simg
    src/img2simg.c
    )

target_link_libraries(emulator_img2simg
    sparse
    )

add_executable(emulator_simg2img
    src/simg2img.c
    )

target_link_libraries(emulator_simg2img
    sparse
    )


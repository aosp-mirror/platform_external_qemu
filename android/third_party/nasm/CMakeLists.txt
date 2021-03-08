cmake_minimum_required(VERSION 3.5)
project(NASM)

if(NOT ANDROID_QEMU2_TOP_DIR)
  get_filename_component(ANDROID_QEMU2_TOP_DIR
                         "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)
  get_filename_component(
    ADD_PATH "${ANDROID_QEMU2_TOP_DIR}/android/build/cmake/" ABSOLUTE)
  list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
endif()
include(android)

set(nasm_compile_warnings
    # The inline functions in NASM's headers flag this.
    "-Wno-unused-function"
    # NASM writes nasm_assert(!"some string literal").
    "-Wno-string-conversion"
    # NASM sometimes redefines macros from its config.h.
    "-Wno-macro-redefined"
    # NASM sometimes compares enums to unsigned integers.
    "-Wno-sign-compare"
    # Microsoft would really like you to use their newer posix implementations
    "-Wno-deprecated-declarations")

# Location of the root of the NASM source.
set(NASM_SRC # cmake-format: sortable
             ${ANDROID_QEMU2_TOP_DIR}/../nasm)

set(ndisasm_src ${NASM_SRC}/disasm/ndisasm.c)
add_library(ndisasm ${ndisasm_src})
target_include_directories(
  ndisasm PUBLIC ${NASM_SRC}/. ${NASM_SRC}/asm ${NASM_SRC}/disasm
                 ${NASM_SRC}/include ${NASM_SRC}/output ${NASM_SRC}/x86)
target_compile_options(ndisasm PUBLIC ${nasm_compile_warnings})

if(UNIX)
  target_compile_definitions(ndisasm PUBLIC "-DHAVE_CONFIG_H")
endif()

# nasmlib
set(nasmlib_src
    ${NASM_SRC}/asm/assemble.c
    ${NASM_SRC}/asm/directbl.c
    ${NASM_SRC}/asm/directiv.c
    ${NASM_SRC}/asm/error.c
    ${NASM_SRC}/asm/eval.c
    ${NASM_SRC}/asm/exprdump.c
    ${NASM_SRC}/asm/exprlib.c
    ${NASM_SRC}/asm/float.c
    ${NASM_SRC}/asm/labels.c
    ${NASM_SRC}/asm/listing.c
    ${NASM_SRC}/asm/parser.c
    ${NASM_SRC}/asm/pptok.c
    ${NASM_SRC}/asm/pragma.c
    ${NASM_SRC}/asm/preproc-nop.c
    ${NASM_SRC}/asm/preproc.c
    ${NASM_SRC}/asm/quote.c
    ${NASM_SRC}/asm/rdstrnum.c
    ${NASM_SRC}/asm/segalloc.c
    ${NASM_SRC}/asm/stdscan.c
    ${NASM_SRC}/asm/strfunc.c
    ${NASM_SRC}/asm/tokhash.c
    ${NASM_SRC}/common/common.c
    ${NASM_SRC}/disasm/disasm.c
    ${NASM_SRC}/disasm/sync.c
    ${NASM_SRC}/macros/macros.c
    ${NASM_SRC}/nasmlib/badenum.c
    ${NASM_SRC}/nasmlib/bsi.c
    ${NASM_SRC}/nasmlib/crc64.c
    ${NASM_SRC}/nasmlib/file.c
    ${NASM_SRC}/nasmlib/filename.c
    ${NASM_SRC}/nasmlib/hashtbl.c
    ${NASM_SRC}/nasmlib/ilog2.c
    ${NASM_SRC}/nasmlib/malloc.c
    ${NASM_SRC}/nasmlib/md5c.c
    ${NASM_SRC}/nasmlib/mmap.c
    ${NASM_SRC}/nasmlib/path.c
    ${NASM_SRC}/nasmlib/perfhash.c
    ${NASM_SRC}/nasmlib/raa.c
    ${NASM_SRC}/nasmlib/rbtree.c
    ${NASM_SRC}/nasmlib/readnum.c
    ${NASM_SRC}/nasmlib/realpath.c
    ${NASM_SRC}/nasmlib/saa.c
    ${NASM_SRC}/nasmlib/srcfile.c
    ${NASM_SRC}/nasmlib/string.c
    ${NASM_SRC}/nasmlib/strlist.c
    ${NASM_SRC}/nasmlib/ver.c
    ${NASM_SRC}/nasmlib/zerobuf.c
    ${NASM_SRC}/output/codeview.c
    ${NASM_SRC}/output/legacy.c
    ${NASM_SRC}/output/nulldbg.c
    ${NASM_SRC}/output/nullout.c
    ${NASM_SRC}/output/outaout.c
    ${NASM_SRC}/output/outas86.c
    ${NASM_SRC}/output/outbin.c
    ${NASM_SRC}/output/outcoff.c
    ${NASM_SRC}/output/outdbg.c
    ${NASM_SRC}/output/outelf.c
    ${NASM_SRC}/output/outform.c
    ${NASM_SRC}/output/outieee.c
    ${NASM_SRC}/output/outlib.c
    ${NASM_SRC}/output/outmacho.c
    ${NASM_SRC}/output/outobj.c
    ${NASM_SRC}/output/outrdf2.c
    ${NASM_SRC}/output/strtbl.c
    ${NASM_SRC}/stdlib/snprintf.c
    ${NASM_SRC}/stdlib/strlcpy.c
    ${NASM_SRC}/stdlib/strnlen.c
    ${NASM_SRC}/stdlib/strrchrnul.c
    ${NASM_SRC}/stdlib/vsnprintf.c
    ${NASM_SRC}/x86/disp8.c
    ${NASM_SRC}/x86/iflag.c
    ${NASM_SRC}/x86/insnsa.c
    ${NASM_SRC}/x86/insnsb.c
    ${NASM_SRC}/x86/insnsd.c
    ${NASM_SRC}/x86/insnsn.c
    ${NASM_SRC}/x86/regdis.c
    ${NASM_SRC}/x86/regflags.c
    ${NASM_SRC}/x86/regs.c
    ${NASM_SRC}/x86/regvals.c)

add_library(nasmlib ${nasmlib_src})
target_link_libraries(nasmlib PRIVATE ndisasm)

set(nasm_src ${NASM_SRC}/asm/nasm.c)
add_executable(nasm ${nasm_src})
target_link_libraries(nasm PRIVATE nasmlib ndisasm)

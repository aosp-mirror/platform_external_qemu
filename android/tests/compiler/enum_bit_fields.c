#include "enum_bit_fields.h"

// QEMU doesn't like to be included from C++, so we just make a C file instead.
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "cpu.h"
#include "exec/exec-all.h"
#include "tcg/tcg.h"


int test_enum_equal() {
    // TCGOp should not have any enum in bitfield weirdness.
    TCGOp tcg;
    memset(&tcg, 0, sizeof(tcg));
    tcg.opc = INDEX_op_qemu_ld_i32;
    return INDEX_op_qemu_ld_i32 == tcg.opc;
}

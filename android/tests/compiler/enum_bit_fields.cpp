#include <string.h>
#include "gtest/gtest.h"

namespace {

typedef enum TCGOpcode {
#define DEF(name, oargs, iargs, cargs, flags) INDEX_op_##name,
#include "tcg/tcg-opc.h"
#undef DEF
    NB_OPS,
} TCGOpcode;

// From tcg.h
typedef struct TCGOp {
    TCGOpcode opc : 8; /*  8 */

    /* Parameters for this opcode.  See below.  */
    unsigned param1 : 4; /* 12 */
    unsigned param2 : 4; /* 16 */

    /* Lifetime data of the operands.  */
    unsigned life : 16; /* 32 */

    /* Arguments for the opcode.  */
    int who_cares;
} TCGOp;

// Assigning into a bitfield doesn't work as expected on windows/msvc.
TEST(CompilerTest, LargeEnumInBitThing) {
    TCGOp tcg;
    memset(&tcg, 0, sizeof(tcg));
    tcg.opc = INDEX_op_qemu_ld_i32;
    EXPECT_EQ(INDEX_op_qemu_ld_i32, tcg.opc);
}
}  // namespace
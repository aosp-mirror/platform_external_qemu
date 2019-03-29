// clang-format off
// Uncomment this line to see crashes in these tests.
// #define USE_CLANG_JMP
#include "qemu/osdep.h"
#include "cpu.h"
#include "compiler_tests.h"
// clang-format on

uint64_t long_jump_preserve_int_params(uint64_t a,
                                       uint64_t b,
                                       uint64_t c,
                                       uint64_t d) {
    CPUState local_cpu;
    if (sigsetjmp(local_cpu.jmp_env, 0x0)) {
        return a + b + c + d;
    }

    siglongjmp(local_cpu.jmp_env, 1);
    return 0;
}


int setjmp_sets_fields() {
    CPUState local_cpu;
    jmp_buf test;
    memset(test, 0xAA, sizeof(test));
    memset(local_cpu.jmp_env, 0xAA, sizeof(local_cpu.jmp_env));
    sigsetjmp(local_cpu.jmp_env, 0x0);
    return memcmp(test, local_cpu.jmp_env, sizeof(test)) != 0;
}

int long_jump_preserve_float_params(float a, float b, float c, float d) {
    CPUState local_cpu;
    float aa = a;
    float bb = b;
    float cc = c;
    float dd = d;
    if (sigsetjmp(local_cpu.jmp_env, 0x0)) {;
        return aa == a && bb == b && cc == c && dd == d;
    }

    siglongjmp(local_cpu.jmp_env, 1);
    return 0;
}

void jump_back(CPUState* local_cpu) {
    siglongjmp(local_cpu->jmp_env, 1);
}

int long_jump_stack_test() {
    CPUState local_cpu;
    memset(local_cpu.jmp_env, 0xAA, sizeof(local_cpu.jmp_env));
     if (sigsetjmp(local_cpu.jmp_env, 0x0)) {
        return 1;
    }

    // This adds a new stack frame, which should work, as we are not
    // overriding our stack frame.
    jump_back(&local_cpu);
    return 0;
}

int long_jump_double_call() {
    CPUState local_cpu;

    if (sigsetjmp(local_cpu.jmp_env, 0x0)) {
        return 0;
    }

    // Overrides the first, so we should return here.
    if (sigsetjmp(local_cpu.jmp_env, 0x0)) {
        return 1;
    }

    siglongjmp(local_cpu.jmp_env, 1);
    return 0;
}

int long_jump_ret_value() {
    CPUState local_cpu;
    int x = sigsetjmp(local_cpu.jmp_env, 0x0);
    if (x) {
        return x == 0xFF;
    }

    siglongjmp(local_cpu.jmp_env, 0xFF);
    return 0;
}



#include "qemu/osdep.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>


int setup_jmp = 0;
int returned = 0;
CPUState* cpu;


void func() {
    if (setup_jmp) {
        longjmp(cpu->jmp_env, 1);
    }
}


int setup_the_jump(CPUState* local_cpu, int local) {
    // Setjmp returns 1 on the longjmp call
    if (setjmp(local_cpu->jmp_env)) {
        // Fake return
        returned = local;
        return 1;
    } else {
        setup_jmp = 1;
    }

    func();
    return 0;
}

int long_jump_stack_test() {
    CPUState local_cpu;
    memset(&local_cpu, 0, sizeof(local_cpu));
    cpu = &local_cpu;
    int jump_res =  setup_the_jump(cpu, 0xCAFE);
    if (returned != 0xCAFE)
      printf("The local variables got trashed.. Which is fine, we can live with that.\n");
    return jump_res;
}

#ifndef __X86_GEN_H__
#define __X86_GEN_H__

#include <stdlib.h>
#include <stdio.h>
#include "qemu-common.h"

typedef uint64_t addr_t;

#define VM_PANIC(x) {\
    printf("%s\n", x); \
    abort(); \
}

#define VM_PANIC_ON(x) {\
    if (x) { \
        printf("%s\n", #x); \
        abort(); \
    } \
}

#define VM_PANIC_EX(...) {\
    printf(__VA_ARGS__); \
    abort(); \
}

#define VM_PANIC_ON_EX(x, ...) {\
    if (x) { \
        printf(__VA_ARGS__); \
        abort(); \
    } \
}

#define ZERO_INIT(obj) memset((void *) &obj, 0, sizeof(obj))

#endif

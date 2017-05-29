#include "handle_reassignment.h"

#define DEFINE_REMAP(type) \
void remap_##type(reassign_##type##_t f, uint32_t count, const type* in, type* out) { \
    for (uint32_t i = 0; i < count; i++) { out[i] = f(in[i]); } \
} \

LIST_REASSIGNABLE_TYPES(DEFINE_REMAP)

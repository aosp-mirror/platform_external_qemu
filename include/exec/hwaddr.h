/* Define hwaddr if it exists.  */

#ifndef TARGPHYS_H
#define TARGPHYS_H

#include <stdint.h>

#ifndef TARGET_PHYS_ADDR_BITS
#define TARGET_PHYS_ADDR_BITS 32
#endif

#ifdef TARGET_PHYS_ADDR_BITS
/* hwaddr is the type of a physical address (its size can
   be different from 'target_ulong').  */

#if TARGET_PHYS_ADDR_BITS == 32
typedef uint32_t hwaddr;
#define TARGET_PHYS_ADDR_MAX UINT32_MAX
#define TARGET_FMT_plx "%08x"
#elif TARGET_PHYS_ADDR_BITS == 64
typedef uint64_t hwaddr;
#define TARGET_PHYS_ADDR_MAX UINT64_MAX
#define TARGET_FMT_plx "%016" PRIx64
#endif
#endif

#endif

/* This file must be included from helper.h */
#ifdef CONFIG_ANDROID_MEMCHECK
/* Hooks to translated BL/BLX. This callback is used to build thread's
 * calling stack.
 * Param:
 *  First pointer contains guest PC where BL/BLX has been found.
 *  Second pointer contains guest PC where BL/BLX will return.
 */
DEF_HELPER_2(on_call, void, i32, i32)
/* Hooks to return from translated BL/BLX. This callback is used to build
 * thread's calling stack.
 * Param:
 *  Pointer contains guest PC where BL/BLX will return.
 */
DEF_HELPER_1(on_ret, void, i32)
#endif  // CONFIG_ANDROID_MEMCHECK
#include "exec/def-helper.h"

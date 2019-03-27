#include <stdint.h>

/* Returns true if the TCG enum is working as expected */
int test_enum_equal();

/* Make sure the long jump is working like expected. */
int long_jump_stack_test();


#define PARAM1 0x11111111
#define PARAM2 0x22222222
#define PARAM3 0x44444444
#define PARAM4 0x88888888

/* Make sure we preserve incoming parameters on longjmp, should return sum(PARAMx)
   ints can be passed into registers vs. stack.
*/
uint64_t long_jump_preserve_int_params(uint64_t a, uint64_t b, uint64_t c, uint64_t d);

/* Make sure we preserve incoming parameters on longjmp, should return true
   Floats can be passed in different registers..
*/
int long_jump_preserve_float_params(float a, float b, float c, float d);

/** Make sure we jump back to the 2nd setjmp instead of the first */
int long_jump_double_call();

/** Make sure we preserve the longjmp return value (can be something else besides 1) */
int long_jump_ret_value();

/** Set jmp will actually store some data */
int setjmp_sets_fields();
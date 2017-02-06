#pragma once
#include "android/utils/compiler.h"

#include <stdlib.h>

ANDROID_BEGIN_HEADER
typedef void (*car_callback_t)(const char*, int, void* context);

extern void set_car_call_back(car_callback_t callback, void* context);

/* initialize car data emulation */
extern void android_car_init(void);
/* send data to android vehicle hal */
extern void android_send_car_data(const char* msg, int msgLen);
ANDROID_END_HEADER
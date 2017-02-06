#pragma once
#include "android/utils/compiler.h"

#include <stdlib.h>
#ifdef __cplusplus
#include <functional>
#endif

ANDROID_BEGIN_HEADER
#ifdef __cplusplus
extern void set_call_back(const std::function<void(const char*, int)>& callback);
#endif

/* initialize car data emulation */
extern void android_car_init(void);
/* send data to android vehicle hal */
extern void android_send_car_data(const char* msg, int msgLen);
ANDROID_END_HEADER
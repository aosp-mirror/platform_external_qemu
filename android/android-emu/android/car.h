#pragma once
#include "android/utils/compiler.h"
#include <stdlib.h>

#ifdef __cplusplus
#include <functional>
#endif

#ifdef __cplusplus
void set_call_back(const std::function<void(const char*, int)>& callback);
#endif

ANDROID_BEGIN_HEADER
/* initialize sensor emulation */
extern void android_car_init( void );

void android_send_car_data(const char* msg, int msgLen);

ANDROID_END_HEADER
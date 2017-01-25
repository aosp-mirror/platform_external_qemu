#pragma once
#include "android/utils/compiler.h"
#include <stdlib.h>

ANDROID_BEGIN_HEADER

/* initialize sensor emulation */
void  android_car_init( void );

void android_send_car_data(const char* msg);
void set_call_back(void (*on_receive_data) (const char*));

ANDROID_END_HEADER
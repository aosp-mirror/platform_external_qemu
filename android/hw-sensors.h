/* Copyright (C) 2009 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#ifndef _android_sensors_h
#define _android_sensors_h

#include "qemu-common.h"

/* initialize sensor emulation */
extern void  android_hw_sensors_init( void );

/* Error definition */
typedef enum{
    SENSOR_NO_SERVICE = -3,
    SENSOR_DISABLED   = -2,
    SENSOR_UNKNOWN    = -1,
} SensorConsoleReturns;

/* NOTE: this list must be the same that the one defined in
 *       the sensors_qemu.c source of the libsensors.goldfish.so
 *       library.
 */
#define  SENSORS_LIST  \
    SENSOR_(ACCELERATION,"acceleration") \
    SENSOR_(MAGNETIC_FIELD,"magnetic-field") \
    SENSOR_(ORIENTATION,"orientation") \
    SENSOR_(TEMPERATURE,"temperature") \

typedef enum {
#define  SENSOR_(x,y)  ANDROID_SENSOR_##x,
    SENSORS_LIST
#undef   SENSOR_
    MAX_SENSORS  /* do not remove */
} AndroidSensor;

extern void  android_hw_sensor_enable( AndroidSensor  sensor );

/* COARSE ORIENTATION VALUES */
typedef enum {
    ANDROID_COARSE_PORTRAIT,
    ANDROID_COARSE_LANDSCAPE
} AndroidCoarseOrientation;

/* change the coarse orientation value */
extern void  android_sensors_set_coarse_orientation( AndroidCoarseOrientation  orient );

/* get sensor values */
extern void android_sensors_get( int sensor_id, float* x, float* y, float* z );

/* set sensor values */
extern void  android_sensors_set( int sensor_id, float x, float y, float z );

/* Get sensor id from sensor name */
extern int get_id_from_sensor_name( char* sensorname );

/* Get sensor name from sensor id */
extern const char* get_sensor_name_from_id( int sensor_id );

/* Get sensor from sensor id */
extern int android_sensors_get_sensor_status( int sensor_id );

#endif /* _android_gps_h */

/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 
#ifndef _ES1XDEBUG_H
#define _ES1XDEBUG_H

#ifndef _ES1XDEFS_H
#       include "es1xDefs.h"
#endif

#    include <stdio.h>

#    define ES1X_LOG(P)              printf P
#    define ES1X_WARNING(P)          printf P
#    define ES1X_ERROR(P)            printf P
// #       define ES1X_LOG(P)
// #       define ES1X_WARNING(P)
// #       define ES1X_ERROR(P)

#include <stdio.h>

const char* es1xEnumToString(GLenum e);

#define ES1X_ENUM_TO_STRING     es1xEnumToString
// #define ES1X_ENUM_TO_STRING(P)

#define ES1X_LOG_CALL(P)        do{                 \
    printf("*** ES1X: ");                           \
    printf P ;                                      \
    printf("\n");                                   \
  } while(0)
// #define ES1X_LOG_CALL(P)

#define ES1X_LOG_UNIFORM_UPDATE(P) printf P
// #define ES1X_LOG_UNIFORM_UPDATE(P)

#endif // _ES1XMATRIX_H

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

 
#include "es1xMemory.h"

#include <memory.h>
#include <stdlib.h>

/*-----------------------------------------------------------------------*/

void es1xMemSet(void* dst, deInt32 value, size_t size)
{
  memset(dst, value, size);
}

/*-----------------------------------------------------------------------*/

void es1xMemCopy(void* dst, const void* src, size_t size)
{
  memcpy(dst, src, size);
}

/*-----------------------------------------------------------------------*/

void* es1xMalloc(size_t size)
{
  return malloc(size);
}

/*-----------------------------------------------------------------------*/

void es1xFree(void* ptr)
{
  free(ptr);
}

/*-----------------------------------------------------------------------*/

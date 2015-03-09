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

 
#include "es1xRouting.h"
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static struct gles2_server_base_t gles2b = { 0 };
struct gles2_server_base_t *gl2b = 0;

void setGLES2base(struct gles2_server_base_t *_gles2b_) {
  void *src = (void *) _gles2b_;
  void *dst = (void *) &gles2b;
  void *ptr = memcpy(dst, src, sizeof(struct gles2_server_base_t));

  if(ptr == dst) {
    gl2b = &gles2b;
    printf("Initialized es1x GLES2 backend @ %p\n", dst);
  } else {
    fprintf(stderr, "Failed to initialize es1x GLES2 backend \n");
  }

  return;
}

#ifdef __cplusplus
}
#endif

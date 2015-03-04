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

#include "GLEScmDispatch.h"
#include <stdio.h>
#include <stdlib.h>

#include "emugl/common/shared_library.h"

gl_decoder_context_t s_gles_cm;
int                  s_gles_cm_enabled;

static emugl::SharedLibrary *s_gles_cm_lib = NULL;

#define DEFAULT_GLES_CM_LIB EMUGL_LIBNAME("GLES_CM_translator")

//
// This function is called only once during initialiation before
// any thread has been created - hence it should NOT be thread safe.
//
bool init_gles_cm_dispatch()
{
  const char *libName = getenv("ANDROID_GLESv1_LIB");
  if (!libName) libName = DEFAULT_GLES_CM_LIB;

  //
  // Load the GLES library
  //
  s_gles_cm_lib = emugl::SharedLibrary::open(libName);
  if (!s_gles_cm_lib) return false;

  //
  // init the GLES dispatch table
  //
  s_gles_cm.initDispatchByName(gles_cm_dispatch_get_proc_func, NULL );
  s_gles_cm_enabled = true;

  return true;
}

//
// This function is called only during initialization before
// any thread has been created - hence it should NOT be thread safe.
//
void *gles_cm_dispatch_get_proc_func(const char *name, void *userData)
{
  if (!s_gles_cm_lib) {
    return NULL;
  }

  void *func = reinterpret_cast<void *>(s_gles_cm_lib->findSymbol(name));
  // if( func != NULL) {
  //   fprintf(stdout, "*** GLEScmDispatch :: GLES_CM %s found @ 0x%p \n", name, func);
  // }

  return func;
}

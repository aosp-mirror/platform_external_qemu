/*
 * Copyright (C) 2011 The Android Open Source Project
 *
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

#include "gles2_dec.h"
#include "gles2_server_base.h"
#include "es1x/es1xRouting.h"
#include "emugl/common/shared_library.h"
#include <GLcommon/TranslatorIfaces.h>
#include <stdio.h>
#include <stdlib.h>

/****************************************************************************/

#define LIB_GLES_V2_NAME EMUGL_LIBNAME("GLES_V2_translator")

#define TRANSLATOR_GETIFACE_NAME "__translator_getIfaces"
static __translator_getGLESIfaceFunc loadIfaces(const char* libName)
{
  emugl::SharedLibrary* libGLES = emugl::SharedLibrary::open(libName);

  if(!libGLES) return NULL;
  __translator_getGLESIfaceFunc func =  (__translator_getGLESIfaceFunc)libGLES->findSymbol(TRANSLATOR_GETIFACE_NAME);
  if(!func) return NULL;
  return func;
}

void init_es2_gl_routing() {
  DBG("%s: load GL interface for ES2 \n", __func__);

  __translator_getGLESIfaceFunc func  = NULL;
  func  = loadIfaces(LIB_GLES_V2_NAME);
  GLESiface *gles_v2_iface = func(NULL);
  gles_v2_iface->initGLESx();

  return;
}

/****************************************************************************/

static emugl::SharedLibrary *s_gles_v2_lib = NULL;

static void *gles_routing_get_proc_func(const char *name, void *userData)
{
  if (!s_gles_v2_lib) {
    return NULL;
  }

  void *func = (void *) s_gles_v2_lib->findSymbol(name);

  return func;
}

/****************************************************************************/

void init_gles_v1tov2_routing() {
  DBG("%s: 1/2 Init GLESv2 Desktop GL dispatcher\n", __func__);

  init_es2_gl_routing();

  DBG("%s: 2/2 Load ES2 for use in es1x\n", __func__);
  const char *libName = LIB_GLES_V2_NAME;
  s_gles_v2_lib = emugl::SharedLibrary::open(libName);
  if (!s_gles_v2_lib) {
    fprintf(stderr, "*** Could not load %s to init GLES routing"
            "through libes1x \n", libName);
    return;
  }

  // init es1x
  gles2_decoder_context_t gles_v2;
  gles_v2.initDispatchByName(gles_routing_get_proc_func, NULL);
  struct gles2_server_base_t *gles_v2_base = static_cast<struct gles2_server_base_t *>(&gles_v2);
  setGLES2base(gles_v2_base);

  return;
}

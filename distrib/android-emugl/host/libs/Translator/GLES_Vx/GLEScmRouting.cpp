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

void init_gl_routing_for_glesv2() {
  printf("*** GLEScmRouting: initializating GL routing table for GLESv2\n");

  __translator_getGLESIfaceFunc func  = NULL;
  func  = loadIfaces(LIB_GLES_V2_NAME);
  GLESiface *gles_v2_iface = func(NULL);
  gles_v2_iface->initGLESx();

  return;
}

/****************************************************************************/

static emugl::SharedLibrary *s_gles_v2_lib = NULL;
bool s_gles_v1tov2_routing_enabled = false;

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
  printf("*** GLEScmRouting: initializating GLESv2 routing table \n");

  // GLES 2.0 must have already been successfully loaded
  // for us to be able to load the GLES 1.x with 1.x-->2.0 transformer
  gles2_decoder_context_t gles_v2;
  const char *libName = LIB_GLES_V2_NAME;

  s_gles_v2_lib = emugl::SharedLibrary::open(libName);
  if (!s_gles_v2_lib) {
    fprintf(stderr, "*** Could not load %s to init GLES routing"
            "through libes1x \n", libName);
    return;
  } else {
    gles_v2.initDispatchByName(gles_routing_get_proc_func, NULL);
  }

  // init es1x
  struct gles2_server_base_t *gles_v2_base = static_cast<struct gles2_server_base_t *>(&gles_v2);
  setGLES2base(gles_v2_base);

  init_gl_routing_for_glesv2();

  s_gles_v1tov2_routing_enabled = true;

  return;
}

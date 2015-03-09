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

#include "GLEScmContext.h"
#include <stdio.h>
#include "es1xContext.h"

GLEScmContext::GLEScmContext():GLESv2Context(), m_es1xContext(NULL) {
  fprintf(stdout, "Create new GLESv1Context\n");
  return;
}

GLEScmContext::~GLEScmContext(){
  if(m_es1xContext != NULL) {
    fprintf(stdout, "Destroy new GLESv1Context; es1xCtx @ %p\n", m_es1xContext);
    es1xContext_destroy(m_es1xContext);
    m_es1xContext = NULL;
  }
}

void GLEScmContext::init() {
  fprintf(stdout, "___ Init GLESv1Context @ %p\n", this);
  if(!m_initialized) {
    GLESv2Context::init(); // will init dispatchFuncs GLES_2_0, set m_initialized
    // TODO: load dispatch-fucntions to libGL for extensios not implemented in GLES2
    //
    // s_glDispatch.dispatchFuncs(GLES_1_to_2); // load only extensions
    //
    // s_glDispatch.dispatchFuncs(GLES_1_1);    // load the whole GLES_1_1 interface
    m_es1xContext = (void *) es1xContext_create();
  }
  fprintf(stdout, "--- Init GLESv1Context @ %p; es1xCtx @ %p\n", this, m_es1xContext);
}

void * GLEScmContext::getES1xContext() const {
  return m_es1xContext;
}

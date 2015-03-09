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

 
#include "es1xDefs.h"
#include "es1xContext.h"
#include "es1xDebug.h"

GL_API void* GL_APIENTRY es1xCreate()
{
  ES1X_LOG_CALL(("es1xCreate\n"));
  return es1xContext_create();
}

GL_API void GL_APIENTRY es1xDestroy(void* _context_)
{
  ES1X_LOG_CALL(("es1xDestroy\n"));
  es1xContext_destroy(_context_);
}

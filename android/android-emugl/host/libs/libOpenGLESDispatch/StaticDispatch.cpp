/*
* Copyright (C) 2020 The Android Open Source Project
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
#include "OpenGLESDispatch/StaticDispatch.h"

#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv1Dispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "OpenGLESDispatch/DispatchTables.h"

#include <string.h>

// Returns functions from s_gles2/s_gles1, for use with static Translator.

void* gles1_dispatch_get_proc_func_static(const char* name) {
    void* func = 0;

#define RETURN_GLES1_STATIC(return_type,function_name,signature,callargs)\
    if (!strcmp(#function_name, name)) { func = (void*)(s_gles1.function_name); } \

    LIST_GLES1_FUNCTIONS(RETURN_GLES1_STATIC, RETURN_GLES1_STATIC);

    return func;
}

void* gles2_dispatch_get_proc_func_static(const char* name) {
    void* func = 0;

#define RETURN_GLES2_STATIC(return_type,function_name,signature,callargs)\
    if (!strcmp(#function_name, name)) { func = (void*)(s_gles2.function_name); } \

    LIST_GLES2_FUNCTIONS(RETURN_GLES2_STATIC, RETURN_GLES2_STATIC);

    return func;
}

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
#include "ClientAPIExts.h"

#include "EglGlobalInfo.h"
#include "GLcommon/GLutils.h"
#include "GLcommon/TranslatorIfaces.h"
#include "ThreadInfo.h"

#include <GLES/gl.h>
#include <GLES/glext.h>

// Define static table to store the function value for each
// client API. functions pointers will get initialized through
// ClientAPIExts::initClientFuncs function after each client API has been
// loaded.
static ClientAPIExts s_client_extensions[MAX_GLES_VERSION - 1];

#define INIT_MEMBER(ret,name,signature,args)  name(NULL),

// Constructor.
ClientAPIExts::ClientAPIExts() :
    LIST_CLIENTAPI_EXTENSIONS_FUNCTIONS(INIT_MEMBER)
    dummy(0)
    {}

void ClientAPIExts::init(const GLESiface* iface) {
#define ASSIGN_METHOD(ret,name,signature,args) \
    this->name = \
          (name ## _t)iface->getProcAddress(#name);

    LIST_CLIENTAPI_EXTENSIONS_FUNCTIONS(ASSIGN_METHOD)
}

// Define implementation for each extension function which checks
// the current context version and calls to the correct client API
// function.
#define RETURN_void /* nothing */
#define RETURN_GLboolean return
#define RETURN_GLenum return

#define RETURN_0_void  return
#define RETURN_0_GLboolean return GL_FALSE
#define RETURN_0_GLenum return 0

#define RETURN_(x)  RETURN_ ## x
#define RETURN_0_(x) RETURN_0_ ## x

#define DEFINE_STATIC_WRAPPER(ret,name,signature,args) \
    static ret _egl_ ## name signature { \
        ThreadInfo* thread  = getThreadInfo(); \
        if (!thread->eglContext.Ptr()) { \
            RETURN_0_(ret); \
        } \
        int idx = (int)thread->eglContext->version() - 1; \
        if (!s_client_extensions[idx].name) { \
            RETURN_0_(ret); \
        } \
        RETURN_(ret) (*s_client_extensions[idx].name) args; \
    }

LIST_CLIENTAPI_EXTENSIONS_FUNCTIONS(DEFINE_STATIC_WRAPPER)

// static
void ClientAPIExts::initClientFuncs(const GLESiface *iface, int idx) {
    s_client_extensions[idx].init(iface);
}

//
// Define a table to map function names to the local _egl_ version of
// the extension function, to be used in eglGetProcAddress.
//
#define DEFINE_TABLE_ENTRY(ret,name,signature,args) \
    { #name, (__translatorMustCastToProperFunctionPointerType)_egl_ ## name },

static struct _client_ext_funcs {
    const char *fname;
    __translatorMustCastToProperFunctionPointerType proc;
} s_client_ext_funcs[] = {
    LIST_CLIENTAPI_EXTENSIONS_FUNCTIONS(DEFINE_TABLE_ENTRY)
};

static const int numExtFuncs = sizeof(s_client_ext_funcs) / 
                               sizeof(s_client_ext_funcs[0]);

//
// returns the __egl_ version of the given extension function name.

// static
__translatorMustCastToProperFunctionPointerType
ClientAPIExts::getProcAddress(const char *fname)
{
    for (int i=0; i<numExtFuncs; i++) {
        if (!strcmp(fname, s_client_ext_funcs[i].fname)) {
            return s_client_ext_funcs[i].proc;
        }
    }
    return NULL;
}


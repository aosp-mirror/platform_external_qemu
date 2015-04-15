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
#ifndef EGL_CLIENT_APIS_EXTS_H
#define EGL_CLIENT_APIS_EXTS_H

#include "ClientAPIExts_functions.h"
#include "GLcommon/TranslatorIfaces.h"

class ClientAPIExts {
public:
    // Constructor.
    ClientAPIExts();

    // This function initialized each entry in the s_client_extensions
    // struct at the givven index using the givven client interface
    static void initClientFuncs(const GLESiface *iface, int idx);

    static __translatorMustCastToProperFunctionPointerType getProcAddress(
            const char *fname);

#define CLIENTAPI_DECLARE_TYPEDEF(ret,name,signature,args) \
    typedef ret (GL_APIENTRY *name ## _t) signature;

    LIST_CLIENTAPI_EXTENSIONS_FUNCTIONS(CLIENTAPI_DECLARE_TYPEDEF)

#define CLIENTAPI_DECLARE_MEMBER(ret,name,signature,args) \
    name ## _t name;

    LIST_CLIENTAPI_EXTENSIONS_FUNCTIONS(CLIENTAPI_DECLARE_MEMBER)
private:
    void init(const GLESiface* iface);

    __translatorMustCastToProperFunctionPointerType resolve(
            const char* functionName);

    int dummy;
};

#endif  // EGL_CLIENT_APIS_EXTS_H

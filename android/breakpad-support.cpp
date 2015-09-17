// Copyright (C) 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/breakpad-support.h"
#include "android/base/testing/TestTempDir.h"

extern "C" void crashme()
{
  volatile int* a = (int*)(NULL);
  *a = 1;
}

#if defined(__linux__)
#include "client/linux/handler/exception_handler.h"

static bool dumpCallback (const google_breakpad::MinidumpDescriptor& descriptor,
                         void* context,
                         bool succeeded)
{
  printf("Dump path: %s\n", descriptor.path());
  //delete(static_cast<android::base::TestTempDir*>(context));
  return succeeded;
}


extern "C" void enable_breakpad(void) {
    //breakpad init
    android::base::TestTempDir* crashdir = new android::base::TestTempDir("android_emulator_crashdir");
    google_breakpad::MinidumpDescriptor descriptor(crashdir->path());
    google_breakpad::ExceptionHandler eh(descriptor, //Minidump
                                       NULL,         //FilterCallback
                                       dumpCallback, //MinidumpCallback
                                       crashdir,         //callback_context
                                       true,         //install_handler
                                       -1);          //server_fd

    crashme();
}
#elif defined(_WIN32)
#include "client/windows/handler/exception_handler.h"

static bool dumpCallback (const wchar_t* dump_path,
                          const wchar_t* minidump_id,
                          void* context,
                          EXCEPTION_POINTERS* exinfo,
                          MDRawAssertionInfo* assertion,
                          bool succeeded) {

    printf("Dump path; %S\n", dump_path);
    //delete(static_cast<android::base::TestTempDir*>(context));
    return succeeded;
}

extern "C" void enable_breakpad(void) {
    //breakpad init
    android::base::TestTempDir* crashdir = new android::base::TestTempDir("android_emulator_crashdir");
    const char * crashpath = crashdir->path();
    wchar_t crashpathWide[crashdir->pathString().size()];
    mbstowcs(&crashpathWide[0], crashpath,crashdir->pathString().size());
    google_breakpad::ExceptionHandler eh(&crashpathWide[0], //dump_path
                                       NULL,                //filter
                                       dumpCallback,        //calback
                                       crashdir,            //callback_context
                                       google_breakpad::ExceptionHandler::HANDLER_ALL); //handler_types

    crashme();
}

#elif defined(__APPLE__)
#include "client/mac/handler/exception_handler.h"

static bool dumpCallback (const char* dump_path,
                          const char* minidump_id,
                          void* context,
                          bool succeeded) {

    printf("Dump path; %s\n", dump_path);
    //delete(static_cast<android::base::TestTempDir*>(context));
    return succeeded;
}

extern "C" void enable_breakpad(void) {
    //breakpad init
    android::base::TestTempDir* crashdir = new android::base::TestTempDir("android_emulator_crashdir");
    std::string crashdirString(crashdir->path());
    google_breakpad::ExceptionHandler eh(crashdirString, //dump_path
                                       NULL,                //filter
                                       dumpCallback,        //calback
                                       crashdir,            //callback_context
                                       true,                //minidump when true
                                       NULL);               //port_name, inprocess when NULL

    crashme();
}

#endif






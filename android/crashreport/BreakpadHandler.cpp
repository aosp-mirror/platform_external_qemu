#include "android/base/system/System.h"
#include "android/utils/compiler.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "android/crashreport/BreakpadUtils.h"

#if defined(__linux__)
#include "client/linux/handler/exception_handler.h"
#elif defined(__APPLE__)
#include "client/mac/handler/exception_handler.h"
#elif defined(_WIN32)
#include "client/windows/handler/exception_handler.h"
#else
#error Breakpad not supported on this platform
#endif


#if defined(_WIN32) && !ANDROID_GCC_PREREQ(4,4)
#  define safe_execv(_filepath,_argv)  execv((_filepath),(const char* const*)(_argv))
#else
#  define safe_execv(_filepath,_argv)  execv((_filepath),(_argv))
#endif

using namespace android;
using namespace android::base;

namespace BreakpadHandler {
namespace {
    google_breakpad::ExceptionHandler* mHandler;

#if defined(__linux__)

    int process_crash(void * argv)
    {
        char * const kEmulatorArgv [] = {
          (char *)BreakpadUtils::get()->getEmulatorPath().c_str(),
          (char *)BreakpadUtils::kEmulatorCrashArg, 
          (char *)argv, NULL};
        safe_execv(BreakpadUtils::get()->getEmulatorPath().c_str(), kEmulatorArgv);
        return 0;
    }

    bool
    dumpCallback(const google_breakpad::MinidumpDescriptor &descriptor,
            void *context,
            bool succeeded)
    {
        fprintf(stderr, "Crash detected. Minidump saved to: %s\n", descriptor.path());
#ifdef CRASHUPLOAD
        //Taken from breakpad
        //Normal fork hanging, workaround using CLONE_VM option
        static const unsigned kChildStackSize = 16000;
        ::google_breakpad::PageAllocator allocator;
        uint8_t* stack = reinterpret_cast<uint8_t*>(allocator.Alloc(kChildStackSize));
        if (stack) {
            //clone() needs the top-most address. (scrub just to be safe)
            stack += kChildStackSize;
            memset(stack - 16, 0, 16);

            const pid_t child = sys_clone(process_crash, stack, CLONE_VFORK | CLONE_FILES | CLONE_FS | CLONE_UNTRACED | CLONE_VM, (void *)descriptor.path(), NULL, NULL, NULL);
            if (child == -1) {
                fprintf(stderr, "Could not spawn clone for crash upload\n");
            }

        }
#endif
        return succeeded;
    }
#elif defined(__APPLE__)
    bool
    dumpCallback(const char *dump_dir,
            const char *minidump_id,
            void *context,
            bool succeeded)
    {
        String path (dump_dir);
        path.append(System::kDirSeparator);
        path.append(minidump_id);
        path.append(".dmp");
        fwprintf(stderr, L"Crash detected. Minidump saved to: %s\n", path.c_str());
#ifdef CRASHUPLOAD
        StringVector cmdvector (BreakpadUtils::get()->getCrashCmdLine());
        cmdvector.append(path);
        System::get()->runCommand(cmdvector, false);
#endif
        return succeeded;
    }
#elif defined(_WIN32)
    bool
    dumpCallback(const wchar_t *dump_path,
            const wchar_t *minidump_id,
            void *context,
            EXCEPTION_POINTERS *exinfo,
            MDRawAssertionInfo *assertion,
            bool succeeded)
    {
        std::wstring wdump_path (dump_path);
        std::string spath;
        spath.assign(wdump_path.begin(), wdump_path.end());
        std::wstring wminidump_id (minidump_id);
        std::string sminidump_id;
        sminidump_id.assign(wminidump_id.begin(), wminidump_id.end());

        String path (spath.c_str());
        path.append(System::kDirSeparator);
        path.append(sminidump_id.c_str());
        path.append(".dmp");
        fwprintf(stderr, L"Crash detected. Minidump saved to: %s\n", path.c_str());
#ifdef CRASHUPLOAD
        StringVector cmdvector (BreakpadUtils::get()->getCrashCmdLine());
        cmdvector.append(path);
        System::get()->runCommand(BreakpadUtils::get()->getCrashCmdLine(), false);
#endif
        return succeeded;
    }
#endif

    bool init () {
        if (!BreakpadUtils::get()->validatePaths()) {
            return false;
        }
        if (!mHandler) {
#if defined(__linux__)
            mHandler =  \
                new google_breakpad::ExceptionHandler(
                google_breakpad::MinidumpDescriptor(
                    BreakpadUtils::get()->getCrashDirectory()),
                nullptr,
                &BreakpadHandler::dumpCallback,
                nullptr,
                true,
                -1);
#elif defined(__APPLE__)
            mHandler =  \
                new google_breakpad::ExceptionHandler(
                BreakpadUtils::get()->getCrashDirectory(),
                nullptr,
                &BreakpadHandler::dumpCallback,
                nullptr,
                true,
                nullptr);
#elif defined(_WIN32)
            mHandler =  \
                new google_breakpad::ExceptionHandler(
                BreakpadUtils::get()->getWCrashDirectory(),
                nullptr,
                &BreakpadHandler::dumpCallback,
                nullptr,
                google_breakpad::ExceptionHandler::HANDLER_ALL);
#endif

        }
        return true;
    }

} //namespace
} //namespace BreakpadHandler

extern "C" bool crashhandler_init(void) {
    return BreakpadHandler::init();
}

#include <stdarg.h>                                 // for va_list, va_start
#include <stdio.h>                                  // for vsnprintf
#include <stdlib.h>                                 // for abort

#include "android/crashreport/CrashReporter.h"      // for CrashReporter
#include "host-common/crash-handler.h"      // for crashhandler_appe...
#include "android/emulation/CpuAccelerator.h"       // for CPU_ACCELERATOR_HVF
#include "host-common/FeatureControl.h"  // for applyCachedServer...
#include "android/metrics/StudioConfig.h"           // for UpdateChannel
#include "android/utils/debug.h"                    // for dfatal

namespace gfxstream {
namespace gl {
struct GLESv2Dispatch;
}  // namespace gl
}  // namespace gfxstream

void crashhandler_die(const char* message) {
    dfatal("%s: fatal: %s", __func__, message);
    abort();
}

void __attribute__((noreturn)) crashhandler_die_format_v(const char* format, va_list args) {
    char message[2048] = {};
    vsnprintf(message, sizeof(message) - 1, format, args);
    crashhandler_die(message);
}

void crashhandler_die_format(const char* format, ...) {
    va_list args;
    va_start(args, format);
    crashhandler_die_format_v(format, args);
}

void crashhandler_append_message(const char* message) {
}

void crashhandler_append_message_format_v(const char* format, va_list args) {
    char message[2048] = {};
    vsnprintf(message, sizeof(message) - 1, format, args);
    crashhandler_append_message(message);
}

void crashhandler_append_message_format(const char* format, ...) {
    va_list args;
    va_start(args, format);
    crashhandler_append_message_format_v(format, args);
    va_end(args);
}

namespace android {

    namespace featurecontrol {
        void applyCachedServerFeaturePatterns() { }
        void asyncUpdateServerFeaturePatterns() { }
    }

namespace crashreport {

    void CrashReporter::GenerateDumpAndDie(const char* msg) {
        dfatal("%s: fatal: %s", __func__, msg);
        abort(); }

    CrashReporter* CrashReporter::get() { return 0; }
}

CpuAccelerator GetCurrentCpuAccelerator() {
#if defined(__linux__)
    return CPU_ACCELERATOR_KVM;
#elif defined(__APPLE__)
    return CPU_ACCELERATOR_HVF;
#else
    return CPU_ACCELERATOR_HAX;
#endif
}

} // namespace android

void tinyepoxy_init(const gfxstream::gl::GLESv2Dispatch* gles, int version) {
    (void)gles;
}

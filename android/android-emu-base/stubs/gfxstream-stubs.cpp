#include "android/base/async/Looper.h"
#include "android/base/async/ThreadLooper.h"
#include "android/crashreport/crash-handler.h"
#include "android/crashreport/CrashReporter.h"
#include "android/crashreport/HangDetector.h"
#include "android/emulation/CpuAccelerator.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/metrics/StudioConfig.h"

#include "OpenGLESDispatch/GLESv2Dispatch.h"

void crashhandler_die(const char* message) {
    fprintf(stderr, "%s: fatal: %s\n", __func__, message);
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
namespace studio {
    UpdateChannel updateChannel() { return UpdateChannel::Stable; }
}

namespace crashreport {

    void CrashReporter::GenerateDumpAndDie(const char* msg) {
        fprintf(stderr, "%s: fatal: %s\n", __func__, msg);
        abort(); }

    CrashReporter* CrashReporter::get() { return 0; }
}

namespace base {

// int Looper::FdWatch::fd() const { return -1; }
// Looper* ThreadLooper::get() { return 0; }

} // namespace base

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

void tinyepoxy_init(const GLESv2Dispatch* gles, int version) {
    (void)gles;
}

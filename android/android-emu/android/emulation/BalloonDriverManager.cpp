#include "android/emulation/BalloonDriverManager.h"

#include "android/base/StringParse.h"
#include "android/base/async/RecurrentTask.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/memory/LazyInstance.h"
#include "android/emulation/control/AdbInterface.h"
#include "android/globals.h"
#include "android/meminfo.h"
#include "android/utils/debug.h"

#include <memory>
#include <sstream>

#if DEBUG >= 1
#include <stdio.h>
#define D(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#define D(...) (void)0
#endif

static void meminfo_callback(const char* msg, int msgLen);

namespace {

// A BalloonDriverManager is an object that runs in the background to inflate
// and deflate the balloon based on available memory in the guest. This feature
// relies on balloon driver handler exported from qemu side.
class BalloonDriverManager {
public:
    using Ptr = std::shared_ptr<BalloonDriverManager>;
    BalloonDriverManager(android::base::Looper* looper,
                         android::base::Looper::Duration checkIntervalMs,
                         balloon_driver_t func)
        : mLooper(looper),
          mCheckIntervalMs(checkIntervalMs),
          mRecurrentTask(
                  looper,
                  [this]() { return android_query_meminfo(meminfo_callback); },
                  mCheckIntervalMs),
          mBalloonFunc(func) {}
    // Unit in Mb
    struct GuestMemUsage {
        unsigned int totalMemory;
        unsigned int availableMemory;
        unsigned int freeMemory;
    };

    void onReceiveMeminfo(const std::string& input) {
        std::istringstream iss(input);
        GuestMemUsage guestMem;
        D("#### onReceiveMeminfo %s", input.c_str());
        if (!parseOutputForGuestMemory(iss, &guestMem) ||
            !adjustBalloonSize(guestMem)) {
            mRecurrentTask.stopAsync();
        }
    }

    void start() { mRecurrentTask.start(); }
    void stop() { mRecurrentTask.stopAndWait(); }
    ~BalloonDriverManager() { stop(); }

    bool parseOutputForGuestMemory(std::istream& stream,
                                   GuestMemUsage* guestMem) {
        if (!stream) {
            return false;
        }
        guestMem->totalMemory = 0;
        guestMem->availableMemory = 0;
        guestMem->freeMemory = 0;
        std::string line;
        int lineCount = 0;
        while (getline(stream, line)) {
            lineCount++;
            if (!line.compare(0, 8, "MemTotal")) {
                unsigned int totalMemory = 0;
                if (1 == android::base::SscanfWithCLocale(
                                 line.c_str(), "MemTotal:%*[ \t]%u kB",
                                 &totalMemory)) {
                    guestMem->totalMemory = totalMemory >> 10;
                } else {
                    return false;
                }
            } else if (!line.compare(0, 7, "MemFree")) {
                unsigned int freeMemory = 0;
                if (1 == android::base::SscanfWithCLocale(
                                 line.c_str(), "MemFree:%*[ \t]%u kB",
                                 &freeMemory)) {
                    guestMem->freeMemory = freeMemory >> 10;
                } else {
                    return false;
                }
            } else if (!line.compare(0, 12, "MemAvailable")) {
                unsigned int availableMemory = 0;
                if (1 == android::base::SscanfWithCLocale(
                                 line.c_str(), "MemAvailable:%*[ \t]%u kB",
                                 &availableMemory)) {
                    guestMem->availableMemory = availableMemory >> 10;
                } else {
                    return false;
                }
            }
            if (lineCount >= 3) {
                break;
            }
        }
        return true;
    }

    bool adjustBalloonSize(GuestMemUsage& guestMem) {
        unsigned int freeMemory = guestMem.freeMemory;
        unsigned int availableMemory = guestMem.availableMemory;
        unsigned int totalMemory = guestMem.totalMemory;
        D("##### freeMemory %u availableMemory %u totalMemory %u free\n",
          freeMemory, availableMemory, totalMemory);
        unsigned int targetTotalMemory = 0;
        if ((double)freeMemory > totalMemory * 0.1) {
            targetTotalMemory = totalMemory - availableMemory / 2;
        } else if ((double)freeMemory < totalMemory * 0.05) {
            targetTotalMemory += availableMemory;
        }
        D("##### target total memory %u \n", targetTotalMemory);
        return (targetTotalMemory != 0)
                       ? mBalloonFunc(targetTotalMemory * 1048576L)
                       : true;
    }

    android::base::Looper* const mLooper;
    const android::base::Looper::Duration mCheckIntervalMs;
    android::base::RecurrentTask mRecurrentTask;
    balloon_driver_t mBalloonFunc;
    DISALLOW_COPY_AND_ASSIGN(BalloonDriverManager);
};

static android::base::LazyInstance<BalloonDriverManager::Ptr>
        sBalloonDriverManager = LAZY_INSTANCE_INIT;
}  // namespace

static void meminfo_callback(const char* msg, int msgLen) {
    if (sBalloonDriverManager.get() != nullptr) {
        sBalloonDriverManager.get()->onReceiveMeminfo(
                std::move(std::string(msg, msgLen)));
    }
}

void android_start_balloon(balloon_driver_t func) {
    sBalloonDriverManager.get() =
            BalloonDriverManager::Ptr(new BalloonDriverManager(
                    android::base::ThreadLooper::get(), 1000 * 60, func));
    D("##### android_start_balloon\n");
    sBalloonDriverManager.get()->start();
}

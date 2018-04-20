#include "android/emulation/BalloonDriverManager.h"

#include "android/base/async/RecurrentTask.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/StringParse.h"
#include "android/emulation/control/AdbInterface.h"
#include "android/globals.h"

#include <istream>
#include <memory>
#include <stdint.h>
#include <stdio.h>
namespace {

// A BalloonDriverManager is an object that runs in the background to inflate and
// deflate the balloon based on available memory in the guest. This feature relies
// on balloon driver handler exported from qemu side. 
class BalloonDriverManager {

public:

    using Ptr = std::shared_ptr<BalloonDriverManager>;
    BalloonDriverManager(unsigned int hwRamSize,
            android::base::Looper* looper,
            android::base::Looper::Duration checkIntervalMs,
            balloon_driver_t func) :
            mHwRamSize(hwRamSize), mLooper(looper), 
            mCheckIntervalMs(checkIntervalMs), 
            mCurrBalloonSize(0),
            mAdbInterface(android::emulation::AdbInterface::create(mLooper)),
            mRecurrentTask(looper,
                     [this]() { return checkGuestMemoryUsage(); },
                     mCheckIntervalMs),
            mBalloonFunc(func) {

    }
    // Unit in Mb
    struct GuestMemUsage {
        unsigned int totalMemory; 
        unsigned int availableMemory; 
        unsigned int freeMemory;
    };

    void start() {
    	mRecurrentTask.start();
    }
    void stop() {
    	mRecurrentTask.stopAndWait();
    }
    ~BalloonDriverManager() {
    	if (mShellCommand) mShellCommand->cancel();
    	stop();
    }

protected:
    BalloonDriverManager(android::base::Looper* looper,
                       android::base::Looper::Duration checkIntervalMs);

private:
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
                if (1 == android::base::SscanfWithCLocale(line.c_str(), "MemTotal:%*[ \t]%u kB", &totalMemory)) {
                    guestMem->totalMemory = totalMemory >> 10;
        		} else {
        			return false;
        		}
            } else if (!line.compare(0, 7, "MemFree")) {
                unsigned int freeMemory = 0;
                if (1 == android::base::SscanfWithCLocale(line.c_str(), "MemFree:%*[ \t]%u kB", &freeMemory)) {
                    guestMem->freeMemory = freeMemory >> 10;
                } else {
                    return false;
                }
        	} else if (!line.compare(0, 12, "MemAvailable")) {
        		unsigned int availableMemory = 0;
                if (1 == android::base::SscanfWithCLocale(line.c_str(), "MemAvailable:%*[ \t]%u kB", &availableMemory)) {
                    guestMem->availableMemory = availableMemory >> 10;
        		} else {
        			return false;
        		}
        	}
        	if(lineCount >= 3) {
        		break;
        	}
        }
        return true;

	}

    bool adjustBalloonSize(GuestMemUsage& guestMem) {
        unsigned int freeMemory = guestMem.freeMemory;
        unsigned int availableMemory = guestMem.availableMemory;
        unsigned int totalMemory = guestMem.totalMemory;
        printf("##### freeMemory %u availableMemory %u totalMemory %u free\n", freeMemory, availableMemory, totalMemory);
        unsigned int targetTotalMemory = 0;
        if ((double) freeMemory > totalMemory * 0.1) {
            targetTotalMemory = totalMemory - availableMemory / 2;
        } else if ((double) freeMemory < totalMemory * 0.05) {
            targetTotalMemory += availableMemory;
        }
        printf("##### target total memory %u \n", targetTotalMemory);
        return (targetTotalMemory != 0)? mBalloonFunc(targetTotalMemory * 1048576L) : true;
    }
    
    bool checkGuestMemoryUsage() {
        if (guest_boot_completed && !mShellCommand) {
        	mShellCommand = mAdbInterface->runAdbCommand({"shell", "cat", "/proc/meminfo"}, 
        		[this](const android::emulation::OptionalAdbCommandResult& result) {
        			if (result && !result->exit_code) {
        				GuestMemUsage guestMem;
        				if (parseOutputForGuestMemory(*(result->output), &guestMem)) {
                            if (!adjustBalloonSize(guestMem))    mRecurrentTask.stopAsync();
                        }
        			}
        			mShellCommand.reset();
    			}, mCheckIntervalMs / 10, true);
        }
        return true;
    }
    unsigned int mHwRamSize; //unit in MB
    android::base::Looper* const mLooper;
    const android::base::Looper::Duration mCheckIntervalMs;
    unsigned int mCurrBalloonSize;
    std::unique_ptr<android::emulation::AdbInterface> mAdbInterface;
    android::base::RecurrentTask mRecurrentTask;
    android::emulation::AdbCommandPtr mShellCommand;
    balloon_driver_t mBalloonFunc;
    DISALLOW_COPY_AND_ASSIGN(BalloonDriverManager);
};


static android::base::LazyInstance<BalloonDriverManager::Ptr> sBalloonDriverManager = LAZY_INSTANCE_INIT;
}

void android_start_balloon(unsigned int hwRamSize, balloon_driver_t func) {
    sBalloonDriverManager.get() = BalloonDriverManager::Ptr(new BalloonDriverManager(hwRamSize, android::base::ThreadLooper::get(), 1000 * 60, func));
    sBalloonDriverManager.get()->start();
}

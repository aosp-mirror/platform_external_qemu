#include "android/emulation/BalloonDriverManager.h"

#include "android/base/async/RecurrentTask.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/StringParse.h"
#include "android/emulation/control/AdbInterface.h"

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

        std::string line;
        int lineCount = 0;
        while (getline(stream, line)) {
        	lineCount++;
        	if (!line.compare(0, 8, "MemTotal")) {
        		unsigned int totalMemory = 0;
        		if (1 == android::base::SscanfWithCLocale(line.c_str(), "MemTotal: %u kB", &totalMemory)) {
        			guestMem->totalMemory = totalMemory << 10;
        		} else {
        			return false;
        		}
        	} else if (!line.compare(0, 12, "MemAvailable")) {
        		unsigned int availableMemory = 0;
        		if (1 == android::base::SscanfWithCLocale(line.c_str(), "MemAvailable: %u kB", &availableMemory)) {
        			guestMem->availableMemory = availableMemory << 10;
        		} else {
        			return false;
        		}
        	}
        	if(lineCount > 3) {
        		break;
        	}
        }
        return true;

	}

    bool adjustBalloonSize(GuestMemUsage& guestMem) {
    	if (mCurrBalloonSize < mHwRamSize) {
    		mCurrBalloonSize += (guestMem.availableMemory / 2);
    		printf("call mBalloonFunc(%u) \n", mHwRamSize - mCurrBalloonSize);
            return mBalloonFunc((mHwRamSize - mCurrBalloonSize) * 1048576L);
    	} 
    	return true;
    }
    
    bool checkGuestMemoryUsage() {
    	
        if (!mShellCommand) {
        	mShellCommand = mAdbInterface->runAdbCommand({"shell", "cat", "/proc/meminfo"}, 
        		[this](const android::emulation::OptionalAdbCommandResult& result) {
        			if (result && !result->exit_code) {
        				GuestMemUsage guestMem;
        				if (parseOutputForGuestMemory(*(result->output), &guestMem)) {
                            if (!adjustBalloonSize(guestMem)) {
                                mRecurrentTask.stopAsync();
                            };
        				} else {
        					printf("### failed to parse output\n");
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
    printf("##### android_start_balloon hwRamSize %u \n", hwRamSize);
    sBalloonDriverManager.get() = BalloonDriverManager::Ptr(new BalloonDriverManager(hwRamSize, android::base::ThreadLooper::get(), 1000 * 60, func));
    sBalloonDriverManager.get()->start();
}
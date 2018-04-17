// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#pragma once

#include "android/utils/compiler.h"
#include "android/base/async/RecurrentTask.h"
#include "android/base/async/ThreadLooper.h"
#include "android/emulation/control/AdbInterface.h"

#include <memory>
#include <stdbool.h>
ANDROID_BEGIN_HEADER

typedef int SLCanReadHandler(void* opaque);


bool android_balloon_start(unsigned int hw_ramSize);





namespace {

// A BalloonDriverManager is an object that runs in the background to inflate and
// deflate the balloon based on available memory in the guest. This feature relies
// on balloon driver handler exported from qemu side. 
class BalloonDriverManager {

public:

    using Ptr = std::shared_ptr<BalloonDriverManager>;
    BalloonDriverManager(unsigned int hwRamSize,
            android::base::Looper* looper,
            android::base::Looper::Duration checkIntervalMs) :
            mHwRamSize(hwRamSize), mLooper(looper), 
            mCheckIntervalMs(checkIntervalMs), 
            mCurrBalloonSize(0),
            mAdbInterface(android::emulation::AdbInterface::create(mLooper)),
            mRecurrentTask(looper,
                     [this]() { return checkGuestMemoryUsage(); },
                     mCheckIntervalMs) {

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
    		//Error *err = NULL;
    		printf("set balloon size to %u MB", mHwRamSize - mCurrBalloonSize);
    		//qmp_balloon(mHwRamSize - mCurrBalloonSize, &err);
    		//if (err)
			//	return false;
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
        					adjustBalloonSize(guestMem);
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
    DISALLOW_COPY_AND_ASSIGN(BalloonDriverManager);
};
ANDROID_END_HEADER
// Copyright 2017 The Android Open Source Project
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

#include "android/base/async/Looper.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/base/threads/Thread.h"
#include "android/emulation/AndroidPipe.h"

#include <memory>
#include <vector>

namespace android {
namespace emulation {

using android::base::System;

// This is a pipe for receiving properties/notifications from the guest.
// There is a background service installed on the guest that will
// forward intent actions + data we are interesting in through this
// pipe.
class AndroidPropertyPipe final : public AndroidPipe {
public:
    class Service final : public AndroidPipe::Service {
    public:
        Service();
        AndroidPipe* create(void* hwPipe, const char* args) override;
    };

    class AndroidPropertyService final : public android::base::Thread {
    public:
        virtual intptr_t main() override;

    private:
        uint64_t mProcessCount = 0;
    };

    AndroidPropertyPipe(void* hwPipe, Service* svc);
    ~AndroidPropertyPipe();

    void onGuestClose(PipeCloseReason reason) override;
    unsigned onGuestPoll() const override;
    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) override;
    int onGuestSend(const AndroidPipeBuffer* buffers, int numBuffers) override;
    void onGuestWantWakeOn(int flags) override {}

    // Initializes the socket connection. Returns true if connection
    // was successful or already open, false otherwise.
    static bool openConnection();
    // Closes the socket connection.
    static void closeConnection();
    // Blocks until receving propstr.
    static bool waitAndroidProperty(const char* propstr,
                                    std::string* outString,
                                    System::Duration timeoutMs = INT64_MAX);
    // Blocks until receiving propstr and value.
    static bool waitAndroidPropertyValue(
            const char* propstr,
            const char* value,
            System::Duration timeoutMs = INT64_MAX);

private:
    // |buff| is assumed to be in the form <pkey>=<pvalue>, where the
    // '=<pvalue>' part is optional.
    // returns true iff <pkey> == |key|. If |outString| is
    // also given, <pvalue> will be written to |outString| if return
    // is true.
    static bool containsProperty(const char* buff,
                                 const char* key,
                                 std::string* outString = nullptr);

    // |buff| is assumed to be in the form <pkey>=<pvalue>, where the
    // '=<pvalue>' part is optional. Returns true iff
    // |buff| == |key|=|value|.
    static bool containsPropertyAndValue(const char* buff,
                                         const char* key,
                                         const char* value);

    // Parses |buff| and writes <pkey> to |key| and <pvalue> to |value|.
    // If no <pvalue>, then value will be set to empty string.
    static void parseProperty(const char* buff,
                              std::string* key,
                              std::string* value);

private:
    static std::unique_ptr<AndroidPropertyService> mAndroidPropertyService;
    static constexpr size_t BUFF_SIZE = 1024;
    static int mInSocket;

    struct ClientSocketInfo {
        std::vector<int> sockets;
        android::base::Lock lock;
    };
    static ClientSocketInfo mClientSocketInfo;
};

void registerAndroidPropertyPipeService();

}  // namespace emulation
}  // namespace android

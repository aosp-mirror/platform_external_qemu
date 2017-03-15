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

#include "android/emulation/AndroidPropertyPipe.h"

#include "android/android-property-pipe.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/sockets/SocketWaiter.h"
#include "android/utils/debug.h"

#include <algorithm>
#include <chrono>

#define D(...) VERBOSE_TID_DPRINT(aprops, __VA_ARGS__)
#define DF(...) VERBOSE_TID_FUNCTION_DPRINT(aprops, __VA_ARGS__)

using android::base::AutoLock;
using android::base::Looper;
using android::base::ScopedPtr;
using android::base::SocketWaiter;

namespace android {
namespace emulation {

// static
int AndroidPropertyPipe::mInSocket = -1;
std::unique_ptr<AndroidPropertyPipe::AndroidPropertyService>
        AndroidPropertyPipe::mAndroidPropertyService(nullptr);
AndroidPropertyPipe::ClientSocketInfo AndroidPropertyPipe::mClientSocketInfo;

AndroidPropertyPipe::AndroidPropertyPipe(void* hwPipe, Service* svc)
    : AndroidPipe(hwPipe, svc) {}

AndroidPropertyPipe::~AndroidPropertyPipe() {}

void AndroidPropertyPipe::onGuestClose(PipeCloseReason reason) {}

unsigned AndroidPropertyPipe::onGuestPoll() const {
    // Guest can always write
    return PIPE_POLL_OUT;
}

int AndroidPropertyPipe::onGuestRecv(AndroidPipeBuffer* buffers,
                                     int numBuffers) {
    // Guest is not supposed to read
    return PIPE_ERROR_IO;
}

int AndroidPropertyPipe::onGuestSend(const AndroidPipeBuffer* buffers,
                                     int numBuffers) {
    int result = 0;

    while (numBuffers > 0) {
        // An Android property has the form of:
        //   <key>=<value>
        // Where <key> is the action part of the intent, and
        // <value> is the data part.

        if (buffers->data && mInSocket >= 0) {
            // forward this data to all waiting threads.
            AutoLock lock(mClientSocketInfo.lock);
            for (int socket : mClientSocketInfo.sockets) {
                android::base::socketSend(socket, (char*)buffers->data,
                                          buffers->size);
            }
            DF("data=%s", buffers->data);
        }
        result += static_cast<int>(buffers->size);
        buffers++;
        numBuffers--;
    }
    return result;
}

// static
bool AndroidPropertyPipe::openConnection() {
    if (mInSocket < 0) {
        // Open server socket on random port
        mInSocket = android::base::socketTcp4LoopbackServer(0);
        if (mInSocket < 0) {
            DF("Error opening socket for AndroidPropertyPipe");
            return false;
        }
        DF("Opened server socket");
        mAndroidPropertyService.reset(new AndroidPropertyService());
        mAndroidPropertyService->start();
    }
    return true;
}

// static
void AndroidPropertyPipe::closeConnection() {
    // Need the following call to unblock accept() call on linux
    // when shutting down.
    android::base::socketShutdownReads(mInSocket);
    android::base::socketClose(mInSocket);
    mInSocket = -1;
    DF("Closed server socket");
}

// static
void AndroidPropertyPipe::parseProperty(const char* buff,
                                        std::string* key,
                                        std::string* value) {
    if (key == nullptr || value == nullptr)
        return;

    std::string str(buff);
    auto pos = str.find("=");
    if (pos != std::string::npos) {
        *key = str.substr(0, pos);
        *value = str.substr(pos + 1);
    } else {
        // No value for key, so string looks like <key>,
        // which is still valid.
        *key = str;
        *value = "";
    }
}

// static
bool AndroidPropertyPipe::containsProperty(const char* buff,
                                           const char* key,
                                           std::string* outString) {
    bool ret = false;
    std::string keystr, valuestr;

    if (buff == nullptr || key == nullptr)
        return false;

    parseProperty(buff, &keystr, &valuestr);
    if (keystr.compare(key) == 0) {
        if (outString != nullptr) {
            *outString = valuestr;
        }
        ret = true;
    }

    return ret;
}

// static
bool AndroidPropertyPipe::containsPropertyAndValue(const char* buff,
                                                   const char* key,
                                                   const char* value) {
    bool ret = false;
    std::string keystr, valuestr;

    if (buff == nullptr || key == nullptr || value == nullptr)
        return false;

    parseProperty(buff, &keystr, &valuestr);
    if (keystr.compare(key) == 0 && valuestr.compare(value) == 0) {
        ret = true;
    }

    return ret;
}

// static
bool AndroidPropertyPipe::waitAndroidProperty(const char* propstr,
                                              std::string* outString,
                                              System::Duration timeoutMs) {
    bool found = false;
    char resstr[BUFF_SIZE];

    // Open client port
    int serverPort = android::base::socketGetPort(mInSocket);
    if (serverPort < 0) {
        DF("Server socket is disconnected.");
        return false;
    }
    int outSocket = android::base::socketTcp4LoopbackClient(serverPort);
    if (outSocket < 0) {
        DF("Unable to create client socket.");
        return false;
    }

    if (timeoutMs == INT64_MAX) {
        DF("key=%s, Blocking mode", propstr);
        // make the socket block on the receive call since we are
        // not timing out.
        android::base::socketSetBlocking(outSocket);
        while (mInSocket >= 0) {
            memset(resstr, 0, BUFF_SIZE);
            auto bytesRead =
                    android::base::socketRecv(outSocket, resstr, BUFF_SIZE);
            if (bytesRead > 0) {
                if (containsProperty(resstr, propstr, outString)) {
                    found = true;
                    break;
                }
            } else {
                D("Disconnected while waiting for data");
                break;
            }
        }
    } else {
        int rs = -1;
        DF("key=%s, timeout=%lu", propstr, timeoutMs);
        // Don't block on reading. Instead, wait on the socket
        // to receive a read event so we can support timeouts.
        ScopedPtr<SocketWaiter> waiter(SocketWaiter::create());
        waiter->update(outSocket, SocketWaiter::kEventRead);

        auto startTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::milliseconds::zero();
        while (elapsed.count() < timeoutMs) {
            if ((rs = waiter->wait(timeoutMs - elapsed.count())) > 0) {
                if (mInSocket >= 0) {
                    memset(resstr, 0, BUFF_SIZE);
                    auto bytesRead = android::base::socketRecv(
                            outSocket, resstr, BUFF_SIZE);
                    if (bytesRead > 0) {
                        if (containsProperty(resstr, propstr, outString)) {
                            found = true;
                            break;
                        }
                    } else {
                        D("Error while trying to read data(%ul)", bytesRead);
                        break;
                    }
                } else {
                    D("Server disconnected while waiting for data");
                    break;
                }
            } else {
                if (rs < 0) {
                    D("Error while waiting for %s", propstr);
                    break;
                }
                D("Timed out waiting for %s", propstr);
            }
            elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime);
        }
        waiter->reset();
    }
    android::base::socketClose(outSocket);

    if (found) {
        D("Found prop=%s", propstr);
    } else {
        D("Timed out or disconnected waiting for %s", propstr);
    }
    return found;
}

// static
bool AndroidPropertyPipe::waitAndroidPropertyValue(const char* propstr,
                                                   const char* value,
                                                   System::Duration timeoutMs) {
    bool found = false;
    char resstr[BUFF_SIZE];

    // Open client port
    int serverPort = android::base::socketGetPort(mInSocket);
    if (serverPort < 0) {
        DF("Server socket is disconnected.");
        return false;
    }
    int outSocket = android::base::socketTcp4LoopbackClient(serverPort);
    if (outSocket < 0) {
        DF("Unable to create client socket.");
        return false;
    }

    if (timeoutMs == INT64_MAX) {
        DF("key=%s, val=%s, Blocking mode", propstr, value);
        // make the socket block on the receive call since we are
        // not timing out.
        android::base::socketSetBlocking(outSocket);
        while (mInSocket >= 0) {
            memset(resstr, 0, BUFF_SIZE);
            auto bytesRead =
                    android::base::socketRecv(outSocket, resstr, BUFF_SIZE);
            if (bytesRead > 0) {
                if (containsPropertyAndValue(resstr, propstr, value)) {
                    found = true;
                    break;
                }
            } else {
                D("Disconnected while waiting for data");
                break;
            }
        }
    } else {
        int rs = -1;
        DF("key=%s, value=%s, timeout=%lu", propstr, value, timeoutMs);
        // Don't block on reading. Instead, wait on the socket
        // to receive a read event so we can support timeouts.
        ScopedPtr<SocketWaiter> waiter(SocketWaiter::create());
        waiter->update(outSocket, SocketWaiter::kEventRead);

        auto startTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::milliseconds::zero();
        while (elapsed.count() < timeoutMs) {
            if ((rs = waiter->wait(timeoutMs - elapsed.count())) > 0) {
                if (mInSocket >= 0) {
                    memset(resstr, 0, BUFF_SIZE);
                    auto bytesRead = android::base::socketRecv(
                            outSocket, resstr, BUFF_SIZE);
                    if (bytesRead > 0) {
                        if (containsPropertyAndValue(resstr, propstr, value)) {
                            found = true;
                            break;
                        }
                    } else {
                        D("Error while trying to read data(%ul)", bytesRead);
                        break;
                    }
                } else {
                    D("Server disconnected while waiting for data");
                    break;
                }
            } else {
                if (rs < 0) {
                    D("Error while waiting for %s", propstr);
                    break;
                }
                D("wait() timed out or error\n");
            }

            elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime);
        }
        waiter->reset();
    }
    android::base::socketClose(outSocket);

    if (found) {
        D("Found prop=%s, value=%s", propstr, value);
    } else {
        D("Timed out or disconnected waiting for %s", propstr);
    }
    return found;
}

void registerAndroidPropertyPipeService() {
    android::AndroidPipe::Service::add(new AndroidPropertyPipe::Service());
}

////////////////////////////////////////////////////////////////////////////////

intptr_t AndroidPropertyPipe::AndroidPropertyService::main() {
    int newSocket;

    while (mInSocket >= 0) {
        newSocket = android::base::socketAcceptAny(mInSocket);
        if (newSocket >= 0) {
            AutoLock lock(mClientSocketInfo.lock);
            mClientSocketInfo.sockets.push_back(newSocket);

            // Do some cleanup; close any sockets that are dead.
            mClientSocketInfo.sockets.erase(
                    std::remove_if(mClientSocketInfo.sockets.begin(),
                                   mClientSocketInfo.sockets.end(),
                                   [](int socket) {
                                       // socket liveness checker
                                       bool ret = android::base::socketSend(
                                                          socket, "!", 1) <= 0;
                                       if (ret) {
                                           android::base::socketClose(socket);
                                       }
                                       return ret;
                                   }),
                    mClientSocketInfo.sockets.end());
        }
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

AndroidPropertyPipe::Service::Service()
    : AndroidPipe::Service("AndroidPropertyPipe") {}

AndroidPipe* AndroidPropertyPipe::Service::create(void* hwPipe,
                                                  const char* args) {
    return new AndroidPropertyPipe(hwPipe, this);
}

}  // namespace emulation
}  // namespace android

void android_init_android_property_pipe(void) {
    android::emulation::registerAndroidPropertyPipeService();
}

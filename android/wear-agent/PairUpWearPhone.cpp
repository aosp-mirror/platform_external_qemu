// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include <android/base/Limits.h>

#include "android/async-utils.h"
#include "android/looper.h"
#include "android/sockets.h"
#include "android/utils/debug.h"
#include "android/wear-agent/PairUpWearPhone.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <queue>
#include <string>
#include <vector>

#define  DPRINT(...)  do {  if (VERBOSE_CHECK(adb)) dprintn(__VA_ARGS__); } while (0)

namespace android {
namespace wear {

class PairUpWearPhoneImpl {
public:
    typedef std::vector<std::string> StrVec;

    PairUpWearPhoneImpl(Looper* looper, const StrVec& devices, int adbHostPort);
    ~PairUpWearPhoneImpl();

    void writeAdbServerSocket();
    void readAdbServerSocket();

    void writeEmulatorConsoleSocket();
    void readEmulatorConsoleSocket();

    bool isDone() const;
    void cleanUp();
private:
    typedef std::queue<std::string>  StrQueue;
    enum CommunicationState {

         PAIRUP_ERROR = 0,

         // when ask adb for device property, need to first ask adb to setup transfer,
         // wait for "OKAY" from adb, then send the query command(which is transferred
         // by adb to the device), and read reply from adb until adb disconnect

         // get product name to distinguish wear from phone
         TO_GET_PRODUCT_NAME_INIT_XFER,
         TO_GET_PRODUCT_NAME_WAIT_OKAY,
         TO_GET_PRODUCT_NAME_SEND_CMD,
         TO_GET_PRODUCT_NAME_READ_REPLY,

         // find out if the phone has wearable app, which is responsible for communication
         // with wear
         TO_GET_WEARABLE_APP_INIT_XFER,
         TO_GET_WEARABLE_APP_WAIT_OKAY,
         TO_GET_WEARABLE_APP_SEND_CMD,
         TO_GET_WEARABLE_APP_READ_REPLY,

         // perform ip forwarding on real device
         // "host-serial:phone-serial-number:forward:tcp:5601;5601"
         TO_FORWARD_IP_SEND_CMD,
         TO_FORWARD_IP_WAIT_OKAY,

         // perform ip redirection on emulator
         // the above command does not work on emulator, so we have to connect to the emulator
         // console and issue command "redir add"
         TO_READ_GREETINGS_FROM_CONSOLE,
         TO_SEND_REDIR_CMD_TO_CONSOLE,

         PAIRUP_SUCCESS,

         PAIRUP_ALREADY_CLEANEDUP
    };
    enum MessageType {
         TRANSFER = 1,
         FORWARDIP,
         PROPERTY,
         WEARABLE_APP_PKG
    };
    static const int READ_BUFFER_SIZE  = 1024;
    static const int WRITE_BUFFER_SIZE = 1024;

    Looper*     mLooper;

    // memeber related to adb host
    int         mAdbHostPort;
    int         mAdbSocket;
    LoopIo      mAdbIo[1];

    // member related to emulator console
    int         mConsolePort;
    int         mConsoleSocket;
    LoopIo      mConsoleIo[1];

    // device categories
    std::string  mDeviceInProbing;
    StrQueue    mUnprobedDevices;
    StrQueue    mWearDevices;
    StrQueue    mPhoneDevices;

    CommunicationState mState;

    // asynchronous reader and writer
    AsyncReader    mAsyncReader[1];
    AsyncWriter    mAsyncWriter[1];
    char           *mReadBuffer;
    char           *mWriteBuffer;
    std::string    mReply;

    // whenever device list changes, call the following
    void updateDevices(const StrVec& devices);
    void startProbeNextDevice();

    // socket connection methods
    void openConnection(int port, int& socketfd, LoopIo* io, LoopIoFunc callback);
    void closeConnection(int& socketfd, LoopIo* io);
    void openAdbConnection();
    void closeAdbConnection();
    void openConsoleConnection();
    void closeConsoleConnection();

    // read and write mode handling methods
    void switchToRead(LoopIo *io);
    void switchToWrite(LoopIo *io);

    // connect wear to phone by setuping port forwarding on emulator or real device
    bool checkForWearDevice();
    bool checkForCompatiblePhone();
    void startConnectWearAndPhone();

    bool startReadConsole();

    // adb asynchronous read/write methods: all return true on success
    // false if need to try again or on error. When error happens,
    // will call startProbeNextDevice
    bool startWriteCommandToAdb(int queryType, const char* message);
    bool completeWriteCommandToAdb();
    bool startReadHeaderFromAdb();
    bool completeReadHeaderFromAdb();
    bool completeReadAllDataFromAdb();
};

static const char kWearableAppName[] = "com.google.android.wearable";

// callback that talks to adb server on the host computer
static void _on_adb_server_socket_fd(void* opaque, int fd, unsigned events) {
    PairUpWearPhoneImpl * pairup = static_cast<PairUpWearPhoneImpl*> (opaque);
    if (!pairup) return;

    if (events & LOOP_IO_READ) {
        pairup->readAdbServerSocket();
    } else if (events & LOOP_IO_WRITE) {
        pairup->writeAdbServerSocket();
    }
    if (pairup->isDone()) pairup->cleanUp();
}

// callback that talks to emulator console
static void _on_emulator_console_socket_fd(void* opaque, int fd, unsigned events) {
    PairUpWearPhoneImpl * pairup = static_cast<PairUpWearPhoneImpl*> (opaque);
    if (!pairup) return;

    if (events & LOOP_IO_READ) {
        pairup->readEmulatorConsoleSocket();
    } else if (events & LOOP_IO_WRITE) {
        pairup->writeEmulatorConsoleSocket();
    }
    if (pairup->isDone()) pairup->cleanUp();
}

//-------------- callbacks for IO with adb server and emulator console
void PairUpWearPhoneImpl::readAdbServerSocket() {

    switch(mState) {
        case TO_GET_PRODUCT_NAME_WAIT_OKAY:
            if (!completeReadHeaderFromAdb()) return;
            if (!startWriteCommandToAdb(PROPERTY, "ro.product.name")) return;
            mState = TO_GET_PRODUCT_NAME_SEND_CMD;
            return;

        case TO_GET_PRODUCT_NAME_READ_REPLY:
            if (!completeReadAllDataFromAdb()) return;
            if (checkForWearDevice()) {
                startConnectWearAndPhone();
                return;
            }
            if (!startWriteCommandToAdb(TRANSFER, mDeviceInProbing.c_str())) return;
            mState = TO_GET_WEARABLE_APP_INIT_XFER;
            return;

        case TO_GET_WEARABLE_APP_WAIT_OKAY:
            if (!completeReadHeaderFromAdb()) return;
            if (!startWriteCommandToAdb(WEARABLE_APP_PKG, kWearableAppName)) return;
            mState = TO_GET_WEARABLE_APP_SEND_CMD;
            return;

        case TO_GET_WEARABLE_APP_READ_REPLY:
            if (!completeReadAllDataFromAdb()) return;
            if (checkForCompatiblePhone()) {
                startConnectWearAndPhone();
                return;
            }
            startProbeNextDevice();
            return;

        case TO_FORWARD_IP_WAIT_OKAY:
            if (!completeReadHeaderFromAdb()) return;
            mState = PAIRUP_SUCCESS;
            return;

        default:
            startProbeNextDevice();
            return;
    }
}

void PairUpWearPhoneImpl::writeAdbServerSocket() {

    switch (mState) {
        case TO_GET_PRODUCT_NAME_INIT_XFER:
            if (!completeWriteCommandToAdb()) return;
            if (!startReadHeaderFromAdb()) return;
            mState = TO_GET_PRODUCT_NAME_WAIT_OKAY;
            return;

        case TO_GET_PRODUCT_NAME_SEND_CMD:
            if (!completeWriteCommandToAdb()) return;
            mState = TO_GET_PRODUCT_NAME_READ_REPLY;
            switchToRead(mAdbIo);
            return;

        case TO_GET_WEARABLE_APP_INIT_XFER:
            if (!completeWriteCommandToAdb()) return;
            if (!startReadHeaderFromAdb()) return;
            mState = TO_GET_WEARABLE_APP_WAIT_OKAY;
            return;

        case TO_GET_WEARABLE_APP_SEND_CMD:
            if (!completeWriteCommandToAdb()) return;
            mState = TO_GET_WEARABLE_APP_READ_REPLY;
            switchToRead(mAdbIo);
            return;

        case TO_FORWARD_IP_SEND_CMD:
            if (!completeWriteCommandToAdb()) return;
            if (!startReadHeaderFromAdb()) return;
            mState = TO_FORWARD_IP_WAIT_OKAY;
            return;

        default:
            startProbeNextDevice();
            return;
    }

}

void PairUpWearPhoneImpl::readEmulatorConsoleSocket() {
    const int size = socket_recv(mConsoleSocket, mReadBuffer, READ_BUFFER_SIZE);

    if (size > 0) {
        mReply.append(mReadBuffer, size);
	DPRINT("read console message: %s\n", mReply.c_str());
        if (mReply.find("OK") != std::string::npos) {
            DPRINT("received mesg from console:\n%s\n", mReply.c_str());
            snprintf(mWriteBuffer, WRITE_BUFFER_SIZE, "redir add tcp:5601:5601\nquit\n");
            DPRINT("sending query to console:\n%s", mWriteBuffer);
            asyncWriter_init(mAsyncWriter, mWriteBuffer, ::strlen(mWriteBuffer), mConsoleIo);
            switchToWrite(mConsoleIo);
            mState = TO_SEND_REDIR_CMD_TO_CONSOLE;
        }
    } else {
	DPRINT("error happened when reading console\n");
        startProbeNextDevice();
    }
}

void PairUpWearPhoneImpl::writeEmulatorConsoleSocket() {
    const int status = asyncWriter_write(mAsyncWriter);

    if (ASYNC_COMPLETE == status) {
        mState = PAIRUP_SUCCESS;
    } else if (ASYNC_ERROR == status) {
        DPRINT("Error: cannot write to console\n");
        startProbeNextDevice();
    }
}

// -------------------------------   Other helper methods
bool PairUpWearPhoneImpl::checkForWearDevice() {
    const bool isWearDevice = (mReply.find("clockwork") != std::string::npos &&
            !strncmp(mDeviceInProbing.c_str(), "emulator-", 9));

    if (isWearDevice) {
        DPRINT("found wear %s\n\n", mDeviceInProbing.c_str());
        mWearDevices.push(mDeviceInProbing);
    }

    return isWearDevice;
}

bool PairUpWearPhoneImpl::checkForCompatiblePhone() {
    const bool isCompatiblePhone = (mReply.find(kWearableAppName) !=  std::string::npos);

    if (isCompatiblePhone) {
        DPRINT("found compatible phone %s\n\n", mDeviceInProbing.c_str());
        mPhoneDevices.push(mDeviceInProbing);
    } else {
        const bool shouldTryAgainLater = (mReply.find("Error:") !=  std::string::npos &&
            mReply.find("Is the system running?") !=  std::string::npos);

        if (shouldTryAgainLater) mUnprobedDevices.push(mDeviceInProbing);
    }

    return isCompatiblePhone;
}

bool PairUpWearPhoneImpl::startReadConsole() {
    openConsoleConnection();

    if (mConsoleSocket < 0) {
        startProbeNextDevice();
        return false;
    } else {
	DPRINT("ready to read console greetings\n");
        switchToRead(mConsoleIo);
        return true;
    }
}

bool PairUpWearPhoneImpl::startWriteCommandToAdb(int queryType, const char* message) {
    if (TRANSFER == queryType || FORWARDIP == queryType) {
        closeAdbConnection();
        openAdbConnection();
    }

    if (mAdbSocket < 0 || !message) {
        startProbeNextDevice();
        return false;
    }

    char buf2[1024];
    switch(queryType) {
        case TRANSFER:
            snprintf(buf2, sizeof(buf2), "host:transport:%s", message);
            break;
        case PROPERTY:
            snprintf(buf2, sizeof(buf2), "shell:getprop %s", message);
            break;
        case WEARABLE_APP_PKG:
            snprintf(buf2, sizeof(buf2), "shell:pm list packages %s", message);
            break;
        case FORWARDIP:
            snprintf(buf2, sizeof(buf2), "host-serial:%s:forward:tcp:5601;tcp:5601", message);
            break;
        default:
            startProbeNextDevice();
            return false;
    }

    snprintf(mWriteBuffer, WRITE_BUFFER_SIZE, "%04x%s", (unsigned)(::strlen(buf2)), buf2);
    DPRINT("sending query '%s' to adb\n", mWriteBuffer);
    asyncWriter_init(mAsyncWriter, mWriteBuffer, ::strlen(mWriteBuffer), mAdbIo);
    switchToWrite(mAdbIo);
    return true;
}

bool PairUpWearPhoneImpl::completeWriteCommandToAdb() {
    int status = asyncWriter_write(mAsyncWriter);

    if (ASYNC_ERROR == status) {
        startProbeNextDevice();
        return false;
    } else if (ASYNC_NEED_MORE == status) {
        return false;
    }
    return true;
}

bool PairUpWearPhoneImpl::startReadHeaderFromAdb() {
    if (mAdbSocket < 0) {
        startProbeNextDevice();
        return false;
    }

    mReadBuffer[4]='\0';
    asyncReader_init(mAsyncReader, mReadBuffer, 4, mAdbIo);
    switchToRead(mAdbIo);
    return true;
}

bool PairUpWearPhoneImpl::completeReadHeaderFromAdb() {
    int status = asyncReader_read(mAsyncReader);

    if (ASYNC_NEED_MORE == status) {
        return false;
    } else if (ASYNC_COMPLETE == status && !strncmp("OKAY", mReadBuffer,4)) {
        DPRINT("received mesg from adb host:%s\n", mReadBuffer);
        return true;
    } else {
        startProbeNextDevice();
        return false;
    }
}

bool PairUpWearPhoneImpl::completeReadAllDataFromAdb() {
    if (mAdbSocket < 0) {
        startProbeNextDevice();
        return false;
    }

    int size = 0;
    char buff[1024];
    size = socket_recv(mAdbSocket, buff, sizeof(buff));
    if (size < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            startProbeNextDevice();
        }
        return false;
    } else if (size > 0) {
        mReply.append(buff, size);
        return false;
    }
    DPRINT("received mesg from adb host:%s\n", mReply.c_str());
    return true;
}

void PairUpWearPhoneImpl::switchToWrite(LoopIo *io) {
    loopIo_dontWantRead(io);
    loopIo_wantWrite(io);
}

void PairUpWearPhoneImpl::switchToRead(LoopIo *io) {
    loopIo_dontWantWrite(io);
    loopIo_wantRead(io);
    mReply.clear();
}

PairUpWearPhoneImpl::PairUpWearPhoneImpl(Looper* looper, const StrVec& devices,
        int adbHostPort) :
        mLooper(looper), mAdbHostPort(adbHostPort), mAdbSocket(-1),
        mAdbIo(), mConsolePort(-1), mConsoleSocket(-1), mConsoleIo(),
        mUnprobedDevices(), mWearDevices(), mPhoneDevices(), mState(PAIRUP_ERROR),
        mAsyncReader(), mAsyncWriter(),
        mReadBuffer(static_cast<char*>(calloc(READ_BUFFER_SIZE,1))),
        mWriteBuffer(static_cast<char*>(calloc(WRITE_BUFFER_SIZE,1))), mReply() {

    updateDevices(devices);
    if (isDone()) cleanUp();
}

PairUpWearPhoneImpl::~PairUpWearPhoneImpl() {
    cleanUp();
    mLooper = 0;
    mAdbHostPort = -1;
    mConsolePort = -1;
}

void PairUpWearPhoneImpl::startProbeNextDevice() {
    closeAdbConnection();
    closeConsoleConnection();

    if (mUnprobedDevices.empty()) {
        mState = PAIRUP_ERROR;
        return;
    }

    mDeviceInProbing = mUnprobedDevices.front();
    mUnprobedDevices.pop();

    if (startWriteCommandToAdb(TRANSFER, mDeviceInProbing.c_str())) {
        mState = TO_GET_PRODUCT_NAME_INIT_XFER;
    } else {
        mState = PAIRUP_ERROR;
    }
}

void PairUpWearPhoneImpl::cleanUp() {
    closeAdbConnection();
    closeConsoleConnection();
    {
        StrQueue q1, q2, q3;
        std::swap(mUnprobedDevices,q1);
        std::swap(mPhoneDevices,q2);
        std::swap(mWearDevices,q3);
    }
    mReply.clear();
    if (mReadBuffer) {
        free(mReadBuffer);
        mReadBuffer = 0;
    }
    if (mWriteBuffer) {
        free(mWriteBuffer);
        mWriteBuffer = 0;
    }
    if (PAIRUP_SUCCESS == mState) {
        DPRINT("\nSUCCESS\n");
    } else if (PAIRUP_ALREADY_CLEANEDUP != mState) {
        DPRINT("\nFAIL\n");
    }
    mState = PAIRUP_ALREADY_CLEANEDUP;
    DPRINT("\nclean up\n\n");
    fflush(stdout); // actually, need to flush stdout
}

bool PairUpWearPhoneImpl::isDone() const {
    return (PAIRUP_ERROR == mState || PAIRUP_SUCCESS == mState);
}

void PairUpWearPhoneImpl::updateDevices(const StrVec& devices) {
    if (devices.size() < 2) {
        DPRINT("Error: There are less than two devices and cannot proceed.\n");
        mState = PAIRUP_ERROR;
        return;
    }
    DPRINT("start paring up wear to phone\n");
    StrQueue deviceQueue;
    for (size_t i = 0; i < devices.size(); ++i) {
        deviceQueue.push(devices[i]);
    }
    std::swap(mUnprobedDevices,deviceQueue);
    startProbeNextDevice();
}

void PairUpWearPhoneImpl::startConnectWearAndPhone() {
    closeAdbConnection();
    closeConsoleConnection();

    if (mPhoneDevices.empty() || mWearDevices.empty()) {
        startProbeNextDevice();
        return;
    }

    std::string phone = mPhoneDevices.front();
    mPhoneDevices.pop();
    const char* emu = "emulator-";
    const int sz = ::strlen(emu);
    if (strncmp(emu, phone.c_str(), sz)) {
        // real phone
        if (startWriteCommandToAdb(FORWARDIP, phone.c_str())) {
            mState = TO_FORWARD_IP_SEND_CMD;
        }
    } else {
        // emulator
        mConsolePort = atoi(phone.c_str() + sz);
        if (startReadConsole()) {
            mState = TO_READ_GREETINGS_FROM_CONSOLE;
        }
    }
}

//-----------------------            helper methods

void PairUpWearPhoneImpl::openAdbConnection() {
    openConnection(mAdbHostPort, mAdbSocket, mAdbIo, _on_adb_server_socket_fd);
    if (mAdbSocket < 0) mState = PAIRUP_ERROR;
}

void PairUpWearPhoneImpl::openConsoleConnection() {
    openConnection(mConsolePort, mConsoleSocket, mConsoleIo, _on_emulator_console_socket_fd);
    if (mConsoleSocket < 0) mState = PAIRUP_ERROR;
}

void PairUpWearPhoneImpl::closeAdbConnection() {
    closeConnection(mAdbSocket, mAdbIo);
}

void PairUpWearPhoneImpl::closeConsoleConnection() {
    closeConnection(mConsoleSocket, mConsoleIo);
}

void PairUpWearPhoneImpl::closeConnection(int& so, LoopIo* io) {
    if (so >= 0) {
        socket_close(so);
        so = -1;
        loopIo_dontWantRead(io);
        loopIo_dontWantWrite(io);
        loopIo_done(io);
    }
}

void PairUpWearPhoneImpl::openConnection(int port, int& so, LoopIo* io, LoopIoFunc callback) {
    so = socket_loopback_client(port, SOCKET_STREAM);
    if (so >= 0) {
        socket_set_nonblock(so);
        loopIo_init(io, mLooper, so, callback, this);
        DPRINT("successfully opened up connection to port %d\n", port);
    } else {
        DPRINT("failed to open up connection to port %d\n", port);
    }
}

// ------------------------------------------------------------------
//
//       PairUpWearPhone  implementation
//
// ------------------------------------------------------------------

PairUpWearPhone::PairUpWearPhone(Looper* looper, const std::vector<std::string>& devices,
        int adbHostPort) :
    mPairUpWearPhoneImpl (new PairUpWearPhoneImpl(looper, devices, adbHostPort)) {
    //VERBOSE_ENABLE(adb);
}

PairUpWearPhone::~PairUpWearPhone() {
    delete mPairUpWearPhoneImpl;
}

bool PairUpWearPhone::isDone() const {
    return mPairUpWearPhoneImpl->isDone();
}

} // namespace wear
} // namespace android

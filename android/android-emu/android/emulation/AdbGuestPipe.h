// Copyright 2016 The Android Open Source Project
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
#include "android/base/async/ScopedSocketWatch.h"
#include "android/base/StringView.h"
#include "android/emulation/AndroidPipe.h"
#include "android/emulation/AdbTypes.h"
#include "android/featurecontrol/feature_control.h"
#include "android/featurecontrol/FeatureControl.h"

#include <vector>
#include <map>

namespace android {
namespace emulation {

// AdbMessage wraps around the raw adb message struct
class AdbMessage {
public:

#define ADB_SYNC 0x434e5953
#define ADB_CNXN 0x4e584e43
#define ADB_OPEN 0x4e45504f
#define ADB_OKAY 0x59414b4f
#define ADB_CLSE 0x45534c43
#define ADB_WRTE 0x45545257
#define ADB_AUTH 0x48545541

#define ADB_AUTH_TOKEN         1
#define ADB_AUTH_SIGNATURE     2
#define ADB_AUTH_RSAPUBLICKEY  3
#define ADB_VERSION 0x01000000

#define MAX_ADB_MESSAGE_PAYLOAD 262144

struct amessage {
    unsigned command;       /* command identifier constant      */
    unsigned arg0;          /* first argument                   */
    unsigned arg1;          /* second argument                  */
    unsigned data_length;   /* length of payload (0 is allowed) */
    unsigned data_check;    /* checksum of data payload         */
    unsigned magic;         /* command ^ 0xffffffff             */
};

struct apacket
{
    amessage mesg;
    uint8_t data[MAX_ADB_MESSAGE_PAYLOAD];
};

    AdbMessage(const char* name);
    ~AdbMessage();

    void read(const AndroidPipeBuffer*, int numBuffers, int count);
private:
    apacket mPacket;
    int mState;
    uint8_t* mCurrPos;
    uint8_t  mBuffer[MAX_ADB_MESSAGE_PAYLOAD + 24];
    uint8_t* mBufferP;
    const char* mName;
    // map target1 to its printing quota: to
    // avoid printing too much logcat messages
    // or push/pull messages.
    std::map<unsigned, int> mPrintQuota;

    void registerMessage();
    int getAllowedBytesToPrint(int bytes);
    void checkForShellExit();
    int getPayloadSize();
    void copyFromBuffer(int count);
    int readPayload(int dataSize);
    void printMessage();
    void printPayload();
    const char* getCommandName(unsigned code);
    void startNewMessage();
    int readHeader(int inputDataSize);
    void readToBuffer(const AndroidPipeBuffer* buffers, int numBuffers, int inputDataSize);
    void parseBuffer(int bufferLength);
};

// AdbGuestPipe implements an AndroidPipe instance corresponding to the guest
// connections from the adbd daemon to the 'qemud:adb' service. Each instance
// is used to implement a single 'transport' between adbd and the host ADB
// server.
//
// The emulator implements two classes to support this: AdbGuestPipe and
// AdbHostListener and everything is linked at runtime like this:
//
//       guest side            |            emulator                   |     host
//
//   adbd <--> /dev/qemu_pipe <--> AdbGuestPipe <--> AdbHostListener <--> ADB Server
//
// There is a little protocol between adbd and the emulator which looks like:
//
//   1) Guest opens /dev/qemu_pipe and writes 'pipe:qemud:adb:<port>\0' to
//      connect to the ADB pipe service, where <port> is a decimal port
//      number (5555 by default). The port number is guest-specific and
//      should be ignored (it is assumed to be here for obsolete reasons).
//
//      -> This ends up calling AdbGuestPipe::Service::create() to create a new
//         instance.
//
//   2) Guest writes 'accept' to tell the emulator to start accepting
//      connections from the ADB server.
//
//   3) Once a connection from the ADB server is established, the emulator
//      should send back 'ok' (or 'ko' / closing the connection to indicate
//      an error).
//
//   4) The guest will write 'start' to indicate it is ready to exchange
//      data with the host ADB server (all data on the pipe should be
//      proxied between the two at this point).
//
// IMPORTANT NOTE: The adbd daemon will typically create a new AdbGuestPipe
//                 instance just after that, but it will only 'accept'
//                 one at a time. For more details see qemu_socket_thread()
//                 in $AOSP/system/core/adb/transport_local.cpp
//
// Usage is the following:
//
//   1) Create a new AdbGuestPipe::Service instance, passing a valid
//      AdbHostAgent instance to it (in practice an AdbHostListener one).
//      Then call AndroidPipe::Service::add() to register it.
//
// NOTE: This must be done at emulation setup time from the QEMU main loop!
//
class AdbGuestPipe : public AndroidPipe {
public:
    using StringView = ::android::base::StringView;
    using ScopedSocket = ::android::base::ScopedSocket;
    using AdbMessage = ::android::emulation::AdbMessage;

    // AndroidPipe::Service class
    class Service : public AndroidPipe::Service, public AdbGuestAgent {
    public:
        Service(AdbHostAgent* hostAgent)
            : AndroidPipe::Service("qemud:adb"), mHostAgent(hostAgent) {}

        // Create a new AdbGuestPipe instance.
        virtual AndroidPipe* create(void* mHwPipe, const char* args) override;

        // Overridden AdbGuestAgent method.
        virtual void onHostConnection(ScopedSocket&& socket) override;

        // Called when a new adb pipe connection is opened by
        // the guest. Note that this does *not* transfer ownership of |pipe|.
        // Technically, AndroidPipe instances are owned by the virtual device.
        // except when onGuestClose() is called (which should destroy it).
        void onPipeOpen(AdbGuestPipe* pipe);

        // Called when the guest closes a pipe. This must delete the instance.
        void onPipeClose(AdbGuestPipe* pipe);

        // search for an item in |mPipes| that is in the WaitingForHostAdbConnection state,
        // remove it from |mPipes| and return it.

        AdbGuestPipe* searchForActivePipe();

        // Resets the current ADB guest pipe connection.
        void resetActiveGuestPipeConnection();

        // For pipes in the middle of deletion to notify the service
        // that they are gone.
        void unregisterActivePipe(AdbGuestPipe* pipe);

    private:

        // remove the pipe from the list
        void removeAdbGuestPipe(AdbGuestPipe* pipe);

        AdbHostAgent* mHostAgent;
        std::vector<AdbGuestPipe*> mPipes;
        AdbGuestPipe* mCurrentActivePipe = nullptr;
    };

    virtual ~AdbGuestPipe();

    // Overridden AndroidPipe methods. Called from the device context.
    virtual void onGuestClose(PipeCloseReason reason) override;
    virtual unsigned onGuestPoll() const override;
    virtual int onGuestRecv(AndroidPipeBuffer* buffers, int count) override;
    virtual int onGuestSend(const AndroidPipeBuffer* buffers,
                            int count) override;
    virtual void onGuestWantWakeOn(int flags) override;

    // Called when a host connection occurs. Transfers ownership of
    // |socket| to the pipe. On success, return true, on error return
    // false and closes the socket.
    void onHostConnection(ScopedSocket&& socket);

    bool isProxyingData() const {
        return mState == State::ProxyingData;
    }

    void resetConnection();

private:
    AdbGuestPipe(void* mHwPipe, Service* service, AdbHostAgent* hostAgent)
        : AndroidPipe(mHwPipe, service), mHostAgent(hostAgent), mReceivedMesg("HOST==>GUEST"),
    mSendingMesg("HOST<==GUEST"){
        setExpectedGuestCommand("accept", State::WaitingForGuestAcceptCommand);
        mPlayStoreImage = android::featurecontrol::isEnabled(android::featurecontrol::PlayStoreImage);
    }

    // Return current service with the right type.
    Service* service() const {
        return static_cast<AdbGuestPipe::Service*>(mService);
    }

    // Each pipe instance can be in one of these states:
    enum class State {
        WaitingForGuestAcceptCommand,
        WaitingForHostAdbConnection,
        SendingAcceptReplyOk,
        WaitingForGuestStartCommand,
        ProxyingData,
        ClosedByGuest,
        ClosedByHost,
    };

    // Used for debugging.
    static const char* toString(State);

    // Called when an i/o event occurs on the host socket.
    // |events| is a mask of FdWatch::kEventXXX flags.
    void onHostSocketEvent(unsigned events);

    // Implement onGuestRecv() and onGuestSend() while proxying the data
    // between the guest and the host.
    int onGuestRecvData(AndroidPipeBuffer* buffers, int count);
    int onGuestSendData(const AndroidPipeBuffer* buffers, int count);

    // Implement onGuestRecv() when sending a reply to the guest.
    int onGuestRecvReply(AndroidPipeBuffer* buffers, int count);

    // Implement onGuestSend() when receiving a command from the guest.
    int onGuestSendCommand(const AndroidPipeBuffer* buffers, int count);

    // Set the reply to send to the guest as |reply|, and change the state
    // to |newState|.
    void setReply(StringView reply, State newState);

    // Set the expected |command| to wait from the guest, and change the
    // state to |newState|.
    void setExpectedGuestCommand(StringView command, State newState);

    // Try to wait for a new host connection. Change the state according to
    // whether one already occured or not.
    void waitForHostConnection();

    // Command/reply buffer and cursor.
    char mBuffer[16];        // the command being accepted or reply being sent.
    size_t mBufferSize = 0;  // size of valid bytes in mBuffer.
    size_t mBufferPos = 0;   // number of matched command bytes on input/output.

    State mState = State::WaitingForGuestAcceptCommand;  // current pipe state.
    android::base::ScopedSocketWatch
            mHostSocket;  // current host socket, if connected.
    AdbHostAgent* mHostAgent = nullptr;
    bool mPlayStoreImage = false;

    android::emulation::AdbMessage mReceivedMesg;
    android::emulation::AdbMessage mSendingMesg;
};

}  // namespace emulation
}  // namespace android

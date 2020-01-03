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

#include "android/emulation/AdbMessageSniffer.h"

#include "android/base/ArraySize.h"
#include "android/emulation/AndroidPipe.h"
#include "android/globals.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <string>
#include <unordered_set>
#include <vector>

static std::atomic<int> host_cts_heart_beat_count{};

namespace android {
namespace emulation {

// An AdbMessageSnifferImpl can intercept the adb message stream and output
// them to a std::ostream when a complete packet is received.
// Will only consume resources if the logging level > 1
class AdbMessageSnifferImpl : public AdbMessageSniffer {
public:
    struct apacket {
        amessage mesg;
        uint8_t data[1024];
    };

    // Creates a new sniffer.
    // |name| The prefix printed for each log message.
    // |level| The requested logging level, only level > 1 will use resources.
    // |oStream| Stream to which to log the messages.
    AdbMessageSnifferImpl(const char* name, int level, std::ostream* oStream);
    ~AdbMessageSnifferImpl() override;

    void read(const AndroidPipeBuffer* /*buffers*/,
              int numBuffers,
              int count) override;

private:
    apacket mPacket;
    int mState;
    uint8_t* mCurrPos;
    std::vector<uint8_t> mBufferVec;
    uint8_t* mBuffer;
    uint8_t* mBufferP;
    const std::string mName;
    int mLevel;
    std::ostream* mLogStream;

    std::unordered_set<unsigned> mDummyShellArg0;

    static const std::array<const std::string, 4> kShellV1;
    static const std::array<const std::string, 3> kShellV2;

    bool packetSeemsValid();
    void grow_buffer_if_needed(int count);
    int getAllowedBytesToPrint(int bytes) const;
    bool checkForDummyShellCommand();
    size_t getPayloadSize() const;
    void copyFromBuffer(size_t count);
    int readPayload(int dataSize);
    void printMessage() const;
    void updateCtsHeartBeatCount();
    void printPayload() const;
    const char* getCommandName(unsigned code) const;
    void startNewMessage();
    int readHeader(int inputDataSize);
    void readToBuffer(const AndroidPipeBuffer* buffers,
                      int numBuffers,
                      int count);
    void parseBuffer(int bufferLength);
};

AdbMessageSniffer* AdbMessageSniffer::create(const char* name,
                                             int level,
                                             std::ostream* oStream) {
    return new AdbMessageSnifferImpl(name, level, oStream);
}

// Shell v1 commands that should not result in a heartbeat.
const std::array<const std::string, 4> AdbMessageSnifferImpl::kShellV1{
        "shell:exit", "shell:getprop",
        "shell:cat /sys/class/power_supply/*/capacity", "framebuffer"};

// Shell v2 commands that should not result in a heartbeat.
const std::array<const std::string, 3> AdbMessageSnifferImpl::kShellV2{
        ",raw:exit", ",raw:getprop",
        ",raw:cat /sys/class/power_supply/*/capacity"};

// the following definition is defined in system/core/adb
#define ADB_SYNC 0x434e5953
#define ADB_CNXN 0x4e584e43
#define ADB_OPEN 0x4e45504f
#define ADB_OKAY 0x59414b4f
#define ADB_CLSE 0x45534c43
#define ADB_WRTE 0x45545257
#define ADB_AUTH 0x48545541

#define ADB_AUTH_TOKEN 1
#define ADB_AUTH_SIGNATURE 2
#define ADB_AUTH_RSAPUBLICKEY 3
#define ADB_VERSION 0x01000000
// Maximum payload for latest adb version (see AOSP //system/core/adb/adb.h)
constexpr size_t MAX_ADB_MESSAGE_PAYLOAD = 1024 * 1024;

AdbMessageSnifferImpl::AdbMessageSnifferImpl(const char* name,
                                             int level,
                                             std::ostream* oStream)
    : mName(name), mLevel(level), mLogStream(oStream) {
    memset(&mPacket, 0, sizeof(apacket));
    startNewMessage();
}

AdbMessageSnifferImpl::~AdbMessageSnifferImpl() = default;

void AdbMessageSnifferImpl::grow_buffer_if_needed(int count) {
    if (mBufferVec.size() < count) {
        mBufferVec.resize(2 * count);
    }
    mBuffer = static_cast<uint8_t*>(&(mBufferVec[0]));
}

void AdbMessageSnifferImpl::readToBuffer(const AndroidPipeBuffer* buffers,
                                         int numBuffers,
                                         int count) {
    mBufferP = mBuffer;

    for (int i = 0; i < numBuffers; ++i) {
        uint8_t* data = buffers[i].data;
        int dataSize = buffers[i].size;
        if (count < dataSize) {
            memcpy(mBufferP, data, count);
            return;
        }
        memcpy(mBufferP, data, dataSize);
        mBufferP += dataSize;
        count -= dataSize;
        if (count <= 0) {
            return;
        }
    }
}

void AdbMessageSnifferImpl::parseBuffer(int bufferLength) {
    mBufferP = mBuffer;
    int count = bufferLength;
    while (count > 0) {
        if (mState == 0) {
            int rd = readHeader(count);
            if (rd <= 0) {
                return;
            }
            count -= rd;
        } else {
            count -= readPayload(count);
        }
    }
}

int AdbMessageSnifferImpl::readHeader(int dataSize) {
    CHECK(mPacket.data - mCurrPos > 0);
    auto need = sizeof(amessage) -
                (mCurrPos - reinterpret_cast<uint8_t*>(&mPacket));
    if (need > dataSize) {
        copyFromBuffer(dataSize);
        return dataSize;
    }
    copyFromBuffer(need);
    if (!packetSeemsValid()) {
        *mLogStream << mName
                    << " Received invalid packet.. Disabling logging.\n";
        mLevel = 0;
        return -1;
    }

    printMessage();
    amessage& msg = mPacket.mesg;
    if (msg.command == ADB_CLSE) {
        mDummyShellArg0.erase(msg.arg0);
    }
    mState = 1;
    return need;
}

void AdbMessageSnifferImpl::updateCtsHeartBeatCount() {
    amessage& msg = mPacket.mesg;
    // OKAY is usually from logcat, ignore it
    if (msg.command != ADB_OKAY && msg.command != ADB_CLSE) {
        if (!checkForDummyShellCommand()) {
            host_cts_heart_beat_count += 1;
            if (mLevel >= 2) {
                *mLogStream << "emulator: cts heartbeat "
                            << host_cts_heart_beat_count.load() << std::endl;
            }
        }
    }
}

bool AdbMessageSnifferImpl::checkForDummyShellCommand() {
    amessage& msg = mPacket.mesg;
    if (msg.command == ADB_OPEN) {
        const std::string purpose = std::string(
                reinterpret_cast<const char*>(mPacket.data), msg.data_length);

        // We only care about shell v1/v2 commands.
        if (purpose.find("shell") != 0 && purpose.find("framebuffer") != 0) {
            return false;
        }

        // True if "substr" in "str"
        auto containsPurpose = [&purpose](const std::string& substr) {
            return purpose.find(substr) != std::string::npos;
        };

        if (std::find(kShellV1.begin(), kShellV1.end(), purpose) !=
                    kShellV1.end() ||
            std::find_if(kShellV2.begin(), kShellV2.end(), containsPurpose) !=
                    kShellV2.end()) {
            mDummyShellArg0.insert(msg.arg0);
            return true;
        }
    }
    return msg.command == ADB_WRTE && mDummyShellArg0.count(msg.arg0) != 0;
}

int AdbMessageSnifferImpl::readPayload(int dataSize) {
    CHECK(mCurrPos - mPacket.data >= 0);

    auto need = getPayloadSize() - (mCurrPos - mPacket.data);
    if (need > dataSize) {
        copyFromBuffer(dataSize);
        return dataSize;
    }
    copyFromBuffer(need);
    updateCtsHeartBeatCount();
    printPayload();
    startNewMessage();
    return need;
}

bool AdbMessageSnifferImpl::packetSeemsValid() {
    // Here you can add a series of checks to validate a packet.
    // For details on valid packets see: AOSP //system/core/adb/adb.h
    return (mPacket.mesg.command == (mPacket.mesg.magic ^ 0xffffffff)) &&
           (mPacket.mesg.data_length <= MAX_ADB_MESSAGE_PAYLOAD);
}

void AdbMessageSnifferImpl::read(const AndroidPipeBuffer* buffers,
                                 int numBuffers,
                                 int count) {
    if (count <= 0 || mLevel < 1) {  // We only track if we are logging..
        return;
    }
    grow_buffer_if_needed(count);
    readToBuffer(buffers, numBuffers, count);
    parseBuffer(count);
}

size_t AdbMessageSnifferImpl::getPayloadSize() const {
    return mPacket.mesg.data_length;
}

void AdbMessageSnifferImpl::copyFromBuffer(size_t count) {
    // guard against overflow
    auto avail =
            sizeof(mPacket) - (mCurrPos - mPacket.data) - sizeof(mPacket.mesg);
    if (avail <= 0) {
        return;
    }

    size_t size = std::min<size_t>(avail, count);
    memcpy(mCurrPos, mBufferP, size);
    mCurrPos += size;
    mBufferP += size;
}

int AdbMessageSnifferImpl::getAllowedBytesToPrint(int bytes) const {
    int allowed = std::min<int>(bytes, sizeof(mPacket.data));
    return allowed;
}

void AdbMessageSnifferImpl::printPayload() const {
    if (mLevel < 2) {
        return;
    }
    const amessage& msg = mPacket.mesg;
    if (msg.command == ADB_OKAY) {
        return;
    }
    if ((msg.command == ADB_WRTE) && (mLevel < 3)) {
        return;
    }
    int length = msg.data_length;
    length = getAllowedBytesToPrint(length);
    if (length <= 0) {
        return;
    }
    const uint8_t* data = mPacket.data;
    for (int i = 0; i < length; ++i) {
        if (i % 1024 == 0) {
            *mLogStream << mName << " ";
        }
        int ch = data[i];
        if (isprint(ch) != 0) {
            *mLogStream << static_cast<char>(ch);
        } else {
            auto flags = mLogStream->flags();
            *mLogStream << "[0x" << std::hex << ch << "]";
            mLogStream->flags(flags);
        }
    }
    *mLogStream << "\n";
}

const char* AdbMessageSnifferImpl::getCommandName(unsigned code) const {
    switch (code) {
        case ADB_SYNC:
            return "SYNC";
        case ADB_CNXN:
            return "CNXN";
        case ADB_OPEN:
            return "OPEN";
        case ADB_CLSE:
            return "CLOSE";
        case ADB_AUTH:
            return "AUTH";
        case ADB_WRTE:
            return "WRITE";
        case ADB_OKAY:
            return "OKAY";
        default:
            return "unknown";
    }
}

void AdbMessageSnifferImpl::printMessage() const {
    if (mLevel < 2) {
        return;
    }

    const amessage& msg = mPacket.mesg;
    if (msg.command == ADB_OKAY) {
        return;
    }
    if ((msg.command == ADB_WRTE) && (mLevel < 3)) {
        return;
    }

    *mLogStream << mName << " command: " << getCommandName(msg.command)
                << ", arg0: " << msg.arg0 << ", arg1: " << msg.arg1
                << ", data length:" << msg.data_length << "\n";
}

void AdbMessageSnifferImpl::startNewMessage() {
    mCurrPos = reinterpret_cast<uint8_t*>(&mPacket);
    mState = 0;
}

}  // namespace emulation
}  // namespace android

extern "C" int get_host_cts_heart_beat_count(void) {
    return host_cts_heart_beat_count.load();
}

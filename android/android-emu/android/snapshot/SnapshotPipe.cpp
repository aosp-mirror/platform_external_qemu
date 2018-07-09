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

#include "android/base/async/ThreadLooper.h"
#include "android/base/synchronization/Lock.h"
#include "android/emulation/AndroidMessagePipe.h"
#include "android/snapshot/interface.h"

#include <assert.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <sstream>
#include <vector>

extern const QAndroidVmOperations* const gQAndroidVmOperations;

namespace {

size_t getReadable(std::iostream& stream) {
    size_t beg = stream.tellg();
    stream.seekg(0, stream.end);
    size_t ret = (size_t)stream.tellg() - beg;
    stream.seekg(beg, stream.beg);
    return ret;
}

class SnapshotPipe : public android::AndroidMessagePipe {
public:
    class Service : public android::AndroidPipe::Service {
    public:
        Service() : android::AndroidPipe::Service("SnapshotPipe") {}
        bool canLoad() const override { return true; }

        virtual android::AndroidPipe* create(void* hwPipe, const char* args)
                override {
            // To avoid complicated synchronization issues, only 1 instance
            // of a SnapshotPipe is allowed at a time
            if (sLock.tryLock()) {
                return new SnapshotPipe(hwPipe, this);
            } else {
                return nullptr;
            }
        }

        android::AndroidPipe* load(void* hwPipe,
                                   const char* args,
                                   android::base::Stream* stream) override {
            __attribute__((unused)) bool success = sLock.tryLock();
            assert(success);
            return new SnapshotPipe(hwPipe, this, stream);
        }
    };
    SnapshotPipe(void* hwPipe,
                 Service* service,
                 android::base::Stream* loadStream = nullptr)
        : android::AndroidMessagePipe(hwPipe, service, decodeAndExecute,
        loadStream) {
        ANDROID_IF_DEBUG(assert(sLock.isLocked()));
        mIsLoad = static_cast<bool>(loadStream);
        if (mIsLoad) {
            DataBuffer recvBuffer;
            int32_t dataSize = sMetaData.size();
            encode(&dataSize, sizeof(dataSize), &recvBuffer);
            encode(sMetaData.data(), dataSize, &recvBuffer);
            resetRecvPayload(std::move(recvBuffer));
            sMetaData.clear();
        }
    }
    ~SnapshotPipe() {
        sLock.unlock();
    }

private:
    template <typename T>
    static void decode(T& obj, const uint8_t*& data) {
        obj = *reinterpret_cast<const T*>(data);
        data += sizeof(T);
    }
    static void decode(void* buf, size_t size, const uint8_t*& data) {
        memcpy(buf, data, size);
        data += size;
    }
    static void encode(void* buf, size_t size, std::vector<uint8_t>* output) {
        uint8_t* buffer = static_cast<uint8_t*>(buf);
        output->insert(output->end(), buffer, buffer + size);
    }
    static void decodeAndExecute(const std::vector<uint8_t>& input,
            std::vector<uint8_t>* output) {
        const uint8_t* data = input.data();
        int32_t opCode;
        decode(opCode, data);
        switch (static_cast<OP>(opCode)) {
            case OP::Save: {
                int32_t nameSize;
                decode(nameSize, data);
                // shared_ptr array not supported until c++17
                std::shared_ptr<std::vector<char>> name(
                        new std::vector<char>(nameSize));
                decode((void*)name->data(), nameSize, data);

                gQAndroidVmOperations->vmStop();
                android::base::ThreadLooper::runOnMainLooper(
                        [name]() {
                            androidSnapshot_save(name->data());
                            gQAndroidVmOperations->vmStart();
                        });
                int32_t toWrite = 0;
                output->resize(sizeof(toWrite));
                encode(&toWrite, sizeof(toWrite), output);
                break;
            }
            case OP::Load: {
                int32_t nameSize;
                decode(nameSize, data);
                std::shared_ptr<std::vector<char>> name(
                        new std::vector<char>(nameSize));
                decode((void*)name->data(), nameSize, data);

                int32_t metaDataSize;
                decode(metaDataSize, data);
                sMetaData.resize(metaDataSize);
                decode((void*)sMetaData.data(), metaDataSize, data);

                android::base::ThreadLooper::runOnMainLooper([name]() {
                    androidSnapshot_load(name->data());
                });
                break;
            }
            default:
                fprintf(stderr,
                        "Error: received bad command from snapshot"
                        " pipe\n");
                assert(0);
        }
    }
    enum class OP : int32_t { Save = 0, Load = 1 };
    bool mIsLoad;
    static std::vector<char> sMetaData;
    static android::base::Lock sLock;
};

std::vector<char> SnapshotPipe::sMetaData = {};
android::base::Lock SnapshotPipe::sLock = {};

}  // namespace

namespace android {
namespace snapshot {
void registerSnapshotPipeService() {
    android::AndroidPipe::Service::add(new SnapshotPipe::Service());
}
}  // namespace snapshot
}  // namespace android

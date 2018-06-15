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
#include "android/emulation/AndroidPipe.h"
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

class SnapshotPipe : public android::AndroidPipe {
public:
    class Service : public android::AndroidPipe::Service {
    public:
        Service() : android::AndroidPipe::Service("SnapshotPipe") {}
        bool canLoad() const override { return true; }

        android::AndroidPipe* create(void* hwPipe, const char* args) override {
            return new SnapshotPipe(hwPipe, this);
        }

        android::AndroidPipe* load(void* hwPipe,
                                   const char* args,
                                   android::base::Stream* stream) override {
            return new SnapshotPipe(hwPipe, this, stream);
        }

        void preLoad(__attribute__((unused))
                     android::base::Stream* stream) override {}

        void preSave(__attribute__((unused))
                     android::base::Stream* stream) override {}
    };
    SnapshotPipe(void* hwPipe,
                 Service* service,
                 android::base::Stream* loadStream = nullptr)
        : AndroidPipe(hwPipe, service) {
        mIsLoad = static_cast<bool>(loadStream);
        if (mIsLoad) {
            fillWriteBuffer();
        }
    }

    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) override {
        assert(getReadable(mWriteBuffer) <= INT_LEAST32_MAX);
        int32_t toWriteSize = static_cast<int32_t>(getReadable(mWriteBuffer));
        // Still failed? we have nothing to read
        if (!toWriteSize) {
            assert(numBuffers >= 1);
            assert(buffers[0].size >= sizeof(int32_t));
            memset(buffers[0].data, 0, sizeof(int32_t));
            return sizeof(int32_t);
        } else {
            int b = 0;
            int retSize = 0;
            while (toWriteSize && b < numBuffers) {
                int32_t bytes = std::min(toWriteSize, (int32_t)buffers[b].size);
                mWriteBuffer.read((char*)buffers[b].data, bytes);
                retSize += bytes;
                toWriteSize -= bytes;
                b++;
            }
            if (!toWriteSize) {
                clearWriteBuffer();
            }
            return retSize;
        }
    }

    int onGuestSend(const AndroidPipeBuffer* buffers, int numBuffers) override {
        int ret = 0;
        for (int i = 0; i < numBuffers; i++) {
            mReadBuffer.write((const char*)buffers[i].data, buffers[i].size);
            ret += (int)buffers[i].size;
        }
        consume();
        return ret;
    }

    unsigned onGuestPoll() const override {
        return PIPE_POLL_IN | PIPE_POLL_OUT;
    }

    void onGuestClose(PipeCloseReason reason) override {}
    void onGuestWantWakeOn(int flags) override {}

private:
    int consume() {
        int ret = 0;
        if (mExpectedRead == -1) {
            if ((int)getReadable(mReadBuffer) >= sizeof(mExpectedRead)) {
                mReadBuffer.read((char*)&mExpectedRead, sizeof(mExpectedRead));
                ret += sizeof(mExpectedRead);
            } else {
                return ret;
            }
        }
        if ((int)getReadable(mReadBuffer) >= mExpectedRead) {
            ret += mExpectedRead;
            // Decode the message
            int32_t opCode;
            mReadBuffer.read((char*)&opCode, sizeof(opCode));
            switch (static_cast<OP>(opCode)) {
                case OP::Save: {
                    clearWriteBuffer();
                    int32_t nameSize;
                    mReadBuffer.read((char*)&nameSize, sizeof(nameSize));
                    // shared_ptr array not supported until c++17
                    std::shared_ptr<std::vector<char>> name(
                            new std::vector<char>(nameSize));
                    mReadBuffer.read(name->data(), nameSize);

                    assert(sizeof(opCode) + sizeof(nameSize) + nameSize ==
                           mExpectedRead);
                    gQAndroidVmOperations->vmStop();
                    android::base::ThreadLooper::runOnMainLooper(
                            [name, this]() {
                                androidSnapshot_save(name->data());
                                gQAndroidVmOperations->vmStart();
                            });
                    break;
                }
                case OP::Load: {
                    int32_t nameSize;
                    mReadBuffer.read((char*)&nameSize, sizeof(nameSize));
                    std::shared_ptr<std::vector<char>> name(
                            new std::vector<char>(nameSize));
                    mReadBuffer.read(name->data(), nameSize);

                    int32_t metaDataSize;
                    mReadBuffer.read((char*)&metaDataSize,
                                     sizeof(metaDataSize));
                    sMetaData.resize(metaDataSize);
                    mReadBuffer.read((char*)sMetaData.data(), metaDataSize);

                    assert(sizeof(opCode) + sizeof(nameSize) + nameSize +
                                   sizeof(metaDataSize) + metaDataSize ==
                           mExpectedRead);

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
        if (getReadable(mReadBuffer) == 0) {
            mReadBuffer.str({});
        }
        return ret;
    }
    void fillWriteBuffer() {
        assert(mIsLoad);
        assert(sMetaData.size() <= INT_LEAST32_MAX);
        clearWriteBuffer();
        int32_t metadataSize = sMetaData.size();
        mWriteBuffer.write((char*)&metadataSize, sizeof(metadataSize));
        mWriteBuffer.write((char*)sMetaData.data(), metadataSize);
        sMetaData.clear();
    }
    void clearWriteBuffer() {
        mWriteBuffer.str({});
    }
    enum class OP : int32_t { Save = 0, Load = 1 };
    int32_t mExpectedRead = -1;
    std::stringstream mReadBuffer;
    std::stringstream mWriteBuffer;
    bool mIsLoad;
    static std::vector<char> sMetaData;
};

// bool SnapshotPipe::sIsLoad = false;
std::vector<char> SnapshotPipe::sMetaData = {};

}  // namespace

namespace android {
namespace snapshot {
void registerSnapshotPipeService() {
    android::AndroidPipe::Service::add(new SnapshotPipe::Service());
}
}  // namespace snapshot
}  // namespace android

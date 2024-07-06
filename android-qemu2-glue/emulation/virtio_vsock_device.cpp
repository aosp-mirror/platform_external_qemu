/* Copyright (C) 2020 The Android Open Source Project
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <deque>
#include <memory>
#include <numeric>
#include <unordered_map>
#include <vector>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" {
#include <hw/virtio/virtio-vsock.h>
#include <qapi/error.h>
#include <qemu/compiler.h>
#include <qemu/iov.h>
#include <qemu/osdep.h>
#include <qemu/typedefs.h>
#include <standard-headers/linux/virtio_vsock.h>
}  // extern "C"

#include <android-qemu2-glue/base/files/QemuFileStream.h>
#include <android-qemu2-glue/emulation/virtio_vsock_transport.h>
#include "aemu/base/logging/Log.h"
#include "host-common/crash-handler.h"
#include "android/emulation/AdbVsockPipe.h"
#include "android/emulation/SocketBuffer.h"
#include "android/emulation/virtio_vsock_device.h"
#include "host-common/VmLock.h"
#include "host-common/android_pipe_base.h"
#include "host-common/android_pipe_common.h"
#include "host-common/android_pipe_device.h"
#include "host-common/FeatureControl.h"
#include "host-common/feature_control.h"

namespace fc = android::featurecontrol;

namespace {
using android::emulation::AdbVsockPipe;
using android::emulation::IVsockNewTransport;
using android::emulation::SocketBuffer;
using android::emulation::vsock_create_transport_connector;
using android::emulation::vsock_load_transport;

constexpr uint32_t VMADDR_CID_HOST = 2;
constexpr uint32_t kHostBufAlloc = 1024 * 1024;
constexpr uint32_t kSrcHostPortMin = 50000;

enum class DstPort {
    Data = 5000,
    Ping = 5001,
    DataNewTransport = 5002,
};

const char* op2str(int op) {
    switch (op) {
        case VIRTIO_VSOCK_OP_REQUEST:
            return "REQUEST";
        case VIRTIO_VSOCK_OP_SHUTDOWN:
            return "SHUTDOWN";
        case VIRTIO_VSOCK_OP_RST:
            return "RST";
        case VIRTIO_VSOCK_OP_RESPONSE:
            return "RESPONSE";
        case VIRTIO_VSOCK_OP_RW:
            return "RW";
        case VIRTIO_VSOCK_OP_CREDIT_UPDATE:
            return "CREDIT_UPDATE";
        case VIRTIO_VSOCK_OP_CREDIT_REQUEST:
            return "CREDIT_REQUEST";
        default:
            return "(bad)";
    }
}

uint64_t makeStreamKey(uint32_t guestPort, uint32_t hostPort) {
    return (uint64_t(guestPort) << 32) | hostPort;
}

::Stream* asCStream(android::base::Stream* stream) {
    return reinterpret_cast<::Stream*>(stream);
}

void saveVector(const std::vector<uint8_t>& v, android::base::Stream* stream) {
    stream->putBe32(v.size());
    stream->write(v.data(), v.size());
}

bool loadVector(std::vector<uint8_t>* v, android::base::Stream* stream) {
    v->resize(stream->getBe32());
    return stream->read(v->data(), v->size()) == v->size();
}

struct VirtIOVSockDev;

struct VSockStream {
    struct ForSnapshotLoad {};

    VSockStream(ForSnapshotLoad, struct VirtIOVSockDev* d)
        : vtblPtr(&vtblNoWake), mDev(d) {}

    VSockStream(struct VirtIOVSockDev* d,
                IVsockHostCallbacks* hostCallbacks,
                std::shared_ptr<IVsockNewTransport> newTransport,
                uint64_t guest_cid,
                uint64_t host_cid,
                uint32_t guest_port,
                uint32_t host_port,
                uint32_t guest_buf_alloc,
                uint32_t guest_fwd_cnt)
        : vtblPtr(&vtblMain),
          mDev(d),
          mHostCallbacks(hostCallbacks),
          mNewTransport(std::move(newTransport)),
          mGuestCid(guest_cid),
          mHostCid(host_cid),
          mGuestPort(guest_port),
          mHostPort(host_port),
          mGuestBufAlloc(guest_buf_alloc),
          mGuestFwdCnt(guest_fwd_cnt) {
        if (!hostCallbacks && !mNewTransport) {
            mPipe = android_pipe_guest_open(this);
        }
    }

    ~VSockStream() {
        if (mPipe) {
            android_pipe_guest_close(mPipe, PIPE_CLOSE_GRACEFUL);
        }
        if (mHostCallbacks) {
            mHostCallbacks->onClose();
        }
    }

    bool ok() const { return mPipe || mHostCallbacks || mNewTransport; }

    void setGuestPos(uint32_t buf_alloc, uint32_t fwd_cnt) {
        mGuestBufAlloc = buf_alloc;
        mGuestFwdCnt = fwd_cnt;
    }

    bool writeGuestToHost(const void* buf, const size_t size) {
        if (mPipe) {
            AndroidPipeBuffer abuf;
            abuf.data = static_cast<uint8_t*>(const_cast<void*>(buf));
            abuf.size = size;

            android_pipe_guest_send(&mPipe, &abuf, 1);
        } else if (mHostCallbacks) {
            mHostCallbacks->onReceive(buf, size);
        } else if (mNewTransport) {
            const uint8_t* buf8 = static_cast<const uint8_t*>(buf);
            size_t to_send = size;
            while (to_send > 0) {
                std::shared_ptr<IVsockNewTransport> nextStage;
                const int n = mNewTransport->writeGuestToHost(buf8, to_send,
                                                              &nextStage);
                if (nextStage) {
                    mNewTransport = std::move(nextStage);
                }
                if (n < 0) {
                    return false;
                }

                buf8 += n;
                to_send -= n;
            }
        } else {
            ::crashhandler_die("%s:%s:%d: No data sink", "VSockStream",
                               __func__, __LINE__);
        }

        mHostFwdCnt += size;
        return true;
    }

    void hostToGuestBufAppend(const void* data, size_t size) {
        mHostToGuestBuf.append(data, size);
    }

    std::tuple<const void*, size_t> hostToGuestBufPeek() const {
        const size_t guestSpaceAvailable =
                mGuestBufAlloc - (mHostSentCnt - mGuestFwdCnt);
        const auto b = mHostToGuestBuf.peek();
        return {b.first, std::min(guestSpaceAvailable, b.second)};
    }

    void hostToGuestBufConsume(size_t size) {
        mHostToGuestBuf.consume(size);
        mHostSentCnt += size;
    }

    void guestConnected() {
        mIsConnected = true;
        if (mHostCallbacks) {
            mHostCallbacks->onConnect();
        }
    }

    void sendOp(enum virtio_vsock_op op);
    void signalWake(bool write);

    void save(android::base::Stream* stream) const {
        stream->putBe64(mGuestCid);
        stream->putBe64(mHostCid);
        stream->putBe32(mGuestPort);
        stream->putBe32(mHostPort);
        stream->putBe32(mGuestBufAlloc);
        stream->putBe32(mGuestFwdCnt);
        stream->putBe32(mHostSentCnt);
        stream->putBe32(mHostFwdCnt);
        stream->putBe32(mSendOpMask);
        stream->putByte(mIsConnected);

        mHostToGuestBuf.save(stream);

        if (mPipe) {
            stream->putByte(1);
            android_pipe_guest_save(mPipe, asCStream(stream));
        } else if (mHostCallbacks) {
            stream->putByte(2);  // see `load` below
        } else if (mNewTransport) {
            stream->putByte(3);
            mNewTransport->save();
        } else {
            ::crashhandler_die("%s:%s:%d unexpected stream state",
                               "VSockStream", __func__, __LINE__);
        }
    }

    enum class LoadResult { Ok, Closed, Error };

    LoadResult load(android::base::Stream* stream) {
        mGuestCid = stream->getBe64();
        mHostCid = stream->getBe64();
        mGuestPort = stream->getBe32();
        mHostPort = stream->getBe32();
        mGuestBufAlloc = stream->getBe32();
        mGuestFwdCnt = stream->getBe32();
        mHostSentCnt = stream->getBe32();
        mHostFwdCnt = stream->getBe32();
        mSendOpMask = stream->getBe32();
        mIsConnected = stream->getByte();

        if (!mHostToGuestBuf.load(stream)) {
            return LoadResult::Error;
        }

        switch (stream->getByte()) {  // see `save` above
            case 1: {
                char force_close_unused = 0;
                mPipe = android_pipe_guest_load(asCStream(stream), this,
                                                &force_close_unused);
                if (!mPipe) {
                    return LoadResult::Closed;
                }
            } break;

            case 2:
                mHostCallbacks = AdbVsockPipe::Service::getHostCallbacks(
                        makeStreamKey(mGuestPort, mHostPort));
                if (!mHostCallbacks) {
                    return LoadResult::Closed;
                }
                break;

            case 3:
                mNewTransport = vsock_load_transport(stream);
                if (!mNewTransport) {
                    return LoadResult::Closed;
                }
                break;

            default:
                return LoadResult::Error;
        }

        vtblPtr = &vtblMain;
        return LoadResult::Ok;
    }

    static void closeFromHostCallback(void* that);

    static void signalWakeCallback(void* that, unsigned flags) {
        static_cast<VSockStream*>(that)->signalWake(true);
    }

    static void signalWakeEmptyCallback(void* that, unsigned flags) {
        // nothing
    }

    static int getPipeIdCallback(void* that) { return 0; }

    const AndroidPipeHwFuncs* vtblPtr;
    struct VirtIOVSockDev* const mDev;
    IVsockHostCallbacks* mHostCallbacks = nullptr;
    void* mPipe = nullptr;
    std::shared_ptr<IVsockNewTransport> mNewTransport;
    uint64_t mGuestCid = 0;
    uint64_t mHostCid = 0;
    uint32_t mGuestPort = 0;
    uint32_t mHostPort = 0;
    uint32_t mGuestBufAlloc = 0;  // guest's buffer size
    uint32_t mGuestFwdCnt = 0;    // how much the guest received
    uint32_t mHostSentCnt = 0;    // how much the host sent
    uint32_t mHostFwdCnt = 0;     // how much the host received
    uint32_t mSendOpMask = 0;     // bitmask of OPs to send
    SocketBuffer mHostToGuestBuf;
    bool mIsConnected = false;

    static const AndroidPipeHwFuncs vtblMain;
    static const AndroidPipeHwFuncs vtblNoWake;
};

const AndroidPipeHwFuncs VSockStream::vtblMain = {
        &closeFromHostCallback,
        &signalWakeCallback,
        &getPipeIdCallback,
};

const AndroidPipeHwFuncs VSockStream::vtblNoWake = {
        &closeFromHostCallback,
        &signalWakeEmptyCallback,
        &getPipeIdCallback,
};

struct virtio_vsock_hdr prepareFrameHeader(const VSockStream* stream,
                                           const enum virtio_vsock_op op,
                                           const uint32_t len) {
    struct virtio_vsock_hdr hdr = {
            .src_cid = stream->mHostCid,
            .dst_cid = stream->mGuestCid,
            .src_port = stream->mHostPort,
            .dst_port = stream->mGuestPort,
            .len = len,
            .type = VIRTIO_VSOCK_TYPE_STREAM,
            .op = static_cast<uint16_t>(op),
            .flags = 0,
            .buf_alloc = kHostBufAlloc,
            .fwd_cnt = stream->mHostFwdCnt,
    };
    return hdr;
}

struct VirtIOVSockDev {
    VirtIOVSockDev(VirtIOVSock* s) : mS(s)
    , mSnaphotLoadFixed_b231345789(fc::isEnabled(fc::VsockSnapshotLoadFixed_b231345789))
    {}

    void closeStreamFromHostLocked(VSockStream* streamWeak) {
        closeStreamLocked(streamWeak->mGuestPort, streamWeak->mHostPort);
    }

    void vqGuestToHostCallbackLocked(VirtIODevice* dev, VirtQueue* vq) {
        vqReadGuestToHostLocked(dev, vq);
        vqWriteHostToGuestLocked();
    }

    void vqWriteHostToGuestLocked() {
        VirtQueue* vq = mS->host_to_guest_vq;

        vqWriteHostToGuestStreamControlFramesLocked(vq) ||
                vqWriteHostToGuestStreamRWFramesLocked(vq) ||
                vqWriteHostToGuestOrphanFramesLocked(vq);

        if (virtio_queue_ready(vq)) {
            virtio_notify(&mS->parent, vq);
        }
    }

    void vqWriteHostToGuestExternalLocked() {
        VirtIODevice* vdev = VIRTIO_DEVICE(mS);

        if (vdev->vm_running) {
            vqWriteHostToGuestLocked();
        }
    }

    uint64_t hostToGuestOpen(const uint32_t guestPort,
                             IVsockHostCallbacks* callbacks) {
        android::RecursiveScopedVmLock lock;

        // this is what we will hear when the guest replies
        const struct virtio_vsock_hdr request = {
                .src_cid = mS->guest_cid,
                .dst_cid = VMADDR_CID_HOST,
                .src_port = guestPort,
                .dst_port = getNextSrcHostPort(),
                .buf_alloc = 0,  // the guest has not sent us this value yet
        };

        const auto stream = createStreamLocked(&request, callbacks, nullptr);
        if (stream) {
            stream->sendOp(VIRTIO_VSOCK_OP_REQUEST);
            vqWriteHostToGuestExternalLocked();
            return makeStreamKey(request.src_port, request.dst_port);
        } else {
            return 0;
        }
    }

    bool hostToGuestClose(const uint64_t key) {
        android::RecursiveScopedVmLock lock;

        auto stream = findStreamLocked(key);
        if (stream) {
            stream->mHostCallbacks = nullptr;  // to prevent recursion in dctor
            stream->sendOp(VIRTIO_VSOCK_OP_SHUTDOWN);
            closeStreamLocked(std::move(stream));
            vqWriteHostToGuestExternalLocked();
            return true;
        } else {
            return false;  // it was closed by the guest side
        }
    }

    size_t hostToGuestSend(const uint64_t key, const void* data, size_t size) {
        android::RecursiveScopedVmLock lock;

        const auto stream = findStreamLocked(key);
        if (stream) {
            stream->hostToGuestBufAppend(data, size);
            vqWriteHostToGuestExternalLocked();
            return size;
        } else {
            return 0;
        }
    }

    bool hostToGuestPing(const uint64_t key) {
        android::RecursiveScopedVmLock lock;

        const auto stream = findStreamLocked(key);
        if (stream) {
            stream->sendOp(VIRTIO_VSOCK_OP_CREDIT_REQUEST);
            vqWriteHostToGuestExternalLocked();
            return true;
        } else {
            return false;
        }
    }

    void save(android::base::Stream* stream) const {
        android::RecursiveScopedVmLock lock;

        stream->putBe32(mStreams.size());
        for (const auto& kv : mStreams) {
            kv.second->save(stream);
        }

        saveVector(mVqGuestToHostBuf, stream);

        stream->putBe32(mHostToGuestOrphanFrames.size());
        for (const auto& f : mHostToGuestOrphanFrames) {
            stream->write(&f, sizeof(f));
        }

        stream->putBe32(mSrcHostPortI);
    }

    bool load(android::base::Stream* stream) {
        android::RecursiveScopedVmLock lock;

        resetDeviceLocked();

        for (uint32_t n = stream->getBe32(); n > 0; --n) {
            auto vstream = std::make_shared<VSockStream>(
                    VSockStream::ForSnapshotLoad(), this);
            switch (vstream->load(stream)) {
                case VSockStream::LoadResult::Ok: {
                    const auto key = makeStreamKey(vstream->mGuestPort,
                                                   vstream->mHostPort);
                    if (!mStreams.insert({key, std::move(vstream)}).second) {
                        return false;
                    }
                } break;

                case VSockStream::LoadResult::Closed:
                    if (mSnaphotLoadFixed_b231345789) {
                        vstream->sendOp(VIRTIO_VSOCK_OP_RST);
                    }
                    break;

                default:
                    return false;
            }
        }

        if (!loadVector(&mVqGuestToHostBuf, stream)) {
            return false;
        }

        for (uint32_t n = stream->getBe32(); n > 0; --n) {
            struct virtio_vsock_hdr hdr;
            if (stream->read(&hdr, sizeof(hdr)) == sizeof(hdr)) {
                mHostToGuestOrphanFrames.push_back(hdr);
            } else {
                return false;
            }
        }

        mSrcHostPortI = stream->getBe32();

        return true;
    }

    void setStatus(const uint8_t status) {
        if ((status & VIRTIO_CONFIG_S_DRIVER_OK) == 0) {
            resetDevice();
        }
    }

    void queueOrphanFrameHostToGuestLocked(const struct virtio_vsock_hdr& hdr) {
        mHostToGuestOrphanFrames.push_back(hdr);
    }

    void queueOrphanFrameHostToGuestLocked(
            const struct virtio_vsock_hdr* request,
            const enum virtio_vsock_op op) {
        const struct virtio_vsock_hdr response = {
                .src_cid = request->dst_cid,
                .dst_cid = request->src_cid,
                .src_port = request->dst_port,
                .dst_port = request->src_port,
                .len = 0,
                .type = VIRTIO_VSOCK_TYPE_STREAM,
                .op = static_cast<uint16_t>(op),
                .flags = 0,
                .buf_alloc = 0,
                .fwd_cnt = 0,
        };

        queueOrphanFrameHostToGuestLocked(response);
    }

private:
    void vqConsumeVirtQueueElement(VirtQueue* vq,
                                   VirtQueueElement* e,
                                   size_t size) {
        virtqueue_push(vq, e, size);
        g_free(e);
    }

    void vqWriteControlFrame(VirtQueue* vq,
                             VirtQueueElement* e,
                             VSockStream* stream,
                             const enum virtio_vsock_op op) {
        const struct virtio_vsock_hdr hdr = prepareFrameHeader(stream, op, 0);
        iov_from_buf(e->in_sg, e->in_num, 0, &hdr, sizeof(hdr));
        vqConsumeVirtQueueElement(vq, e, sizeof(hdr));
        stream->mSendOpMask &= ~(1u << op);
    }

    bool vqWriteHostToGuestStreamControlFramesLocked(VirtQueue* vq) {
        for (auto& kv : mStreams) {
            auto stream = &*kv.second;

            while (stream->mSendOpMask) {
                if (!virtio_queue_ready(vq)) {
                    return true;
                }

                VirtQueueElement* e = static_cast<VirtQueueElement*>(
                        virtqueue_pop(vq, sizeof(VirtQueueElement)));
                if (!e) {
                    return true;
                }

                const size_t eSize = iov_size(e->in_sg, e->in_num);
                if (eSize < sizeof(struct virtio_vsock_hdr)) {
                    vqConsumeVirtQueueElement(vq, e, 0);
                    return true;
                }

                if (stream->mSendOpMask & (1u << VIRTIO_VSOCK_OP_REQUEST)) {
                    vqWriteControlFrame(vq, e, stream, VIRTIO_VSOCK_OP_REQUEST);
                } else if (stream->mSendOpMask &
                           (1u << VIRTIO_VSOCK_OP_RESPONSE)) {
                    vqWriteControlFrame(vq, e, stream,
                                        VIRTIO_VSOCK_OP_RESPONSE);
                    stream->mIsConnected = true;
                } else if (stream->mSendOpMask &
                           (1u << VIRTIO_VSOCK_OP_CREDIT_UPDATE)) {
                    vqWriteControlFrame(vq, e, stream,
                                        VIRTIO_VSOCK_OP_CREDIT_UPDATE);
                } else if (stream->mSendOpMask &
                           (1u << VIRTIO_VSOCK_OP_CREDIT_REQUEST)) {
                    vqWriteControlFrame(vq, e, stream,
                                        VIRTIO_VSOCK_OP_CREDIT_REQUEST);
                } else {
                    ::crashhandler_die("%s:%s:%d unexpected mSendOpMask=%x",
                                       "VirtIOVSockDev", __func__, __LINE__,
                                       stream->mSendOpMask);
                }
            }
        }

        return false;
    }

    bool vqWriteHostToGuestStreamRWFramesLocked(VirtQueue* vq) {
        for (auto& kv : mStreams) {
            auto stream = &*kv.second;

            if (stream->mIsConnected) {
                while (true) {
                    const auto [data, dataSize] = stream->hostToGuestBufPeek();
                    if (!dataSize) {
                        break;
                    }

                    if (!virtio_queue_ready(vq)) {
                        return true;
                    }

                    VirtQueueElement* e = static_cast<VirtQueueElement*>(
                            virtqueue_pop(vq, sizeof(VirtQueueElement)));
                    if (!e) {
                        return true;
                    }

                    const size_t eSize = iov_size(e->in_sg, e->in_num);
                    if (eSize < sizeof(struct virtio_vsock_hdr)) {
                        vqConsumeVirtQueueElement(vq, e, 0);
                        return true;
                    }

                    const struct virtio_vsock_hdr hdr = prepareFrameHeader(
                            stream, VIRTIO_VSOCK_OP_RW,
                            std::min(dataSize,
                                     eSize - sizeof(struct
                                                    virtio_vsock_hdr)));

                    iov_from_buf(e->in_sg, e->in_num, 0, &hdr, sizeof(hdr));
                    iov_from_buf(e->in_sg, e->in_num, sizeof(hdr), data,
                                 hdr.len);
                    stream->hostToGuestBufConsume(hdr.len);

                    vqConsumeVirtQueueElement(vq, e, sizeof(hdr) + hdr.len);
                }
            }
        }

        return false;
    }

    bool vqWriteHostToGuestOrphanFramesLocked(VirtQueue* vq) {
        while (!mHostToGuestOrphanFrames.empty()) {
            if (!virtio_queue_ready(vq)) {
                return true;
            }

            VirtQueueElement* e = static_cast<VirtQueueElement*>(
                    virtqueue_pop(vq, sizeof(VirtQueueElement)));
            if (!e) {
                return true;
            }

            const size_t eSize = iov_size(e->in_sg, e->in_num);
            if (eSize < sizeof(struct virtio_vsock_hdr)) {
                vqConsumeVirtQueueElement(vq, e, 0);
                return true;
            }

            const struct virtio_vsock_hdr& hdr =
                    mHostToGuestOrphanFrames.front();
            iov_from_buf(e->in_sg, e->in_num, 0, &hdr, sizeof(hdr));
            vqConsumeVirtQueueElement(vq, e, sizeof(hdr));

            mHostToGuestOrphanFrames.pop_front();
        }

        return false;
    }

    void resetDeviceLocked() {
        mStreams.clear();
        mVqGuestToHostBuf.clear();
        mHostToGuestOrphanFrames.clear();
    }

    void resetDevice() {
        android::RecursiveScopedVmLock lock;
        resetDeviceLocked();
    }

    std::shared_ptr<VSockStream> createStreamLocked(
            const struct virtio_vsock_hdr* request,
            IVsockHostCallbacks* callbacks,
            std::shared_ptr<IVsockNewTransport> newTransport) {
        auto stream = std::make_shared<VSockStream>(
                this, callbacks, std::move(newTransport), request->src_cid,
                request->dst_cid, request->src_port, request->dst_port,
                request->buf_alloc, request->fwd_cnt);
        if (stream->ok()) {
            const auto r = mStreams.insert(
                    {makeStreamKey(request->src_port, request->dst_port), {}});
            if (r.second) {
                r.first->second = stream;
                return stream;
            } else {
                return nullptr;
            }
        } else {
            return nullptr;
        }
    }

    std::shared_ptr<VSockStream> createNewStreamLocked(
            const struct virtio_vsock_hdr* request) {
        const uint64_t key =
                makeStreamKey(request->src_port, request->dst_port);
        return createStreamLocked(request, nullptr,
                                  vsock_create_transport_connector(key));
    }

    std::shared_ptr<VSockStream> findStreamLocked(uint64_t key) {
        const auto i = mStreams.find(key);
        if (i == mStreams.end()) {
            return nullptr;
        } else {
            return i->second;
        }
    }

    std::shared_ptr<VSockStream> findStreamLocked(uint32_t guestPort,
                                                  uint32_t hostPort) {
        return findStreamLocked(makeStreamKey(guestPort, hostPort));
    }

    bool closeStreamLocked(const uint64_t key) {
        return mStreams.erase(key) > 0;
    }

    void closeStreamLocked(const uint32_t guestPort, const uint32_t hostPort) {
        closeStreamLocked(makeStreamKey(guestPort, hostPort));
    }

    void closeStreamLocked(std::shared_ptr<VSockStream> stream) {
        closeStreamLocked(stream->mGuestPort, stream->mHostPort);
    }

    void vqParseGuestToHostRequestLocked(
            const struct virtio_vsock_hdr* request) {
        if (request->type != VIRTIO_VSOCK_TYPE_STREAM) {
            return;
        }

        if (request->op == VIRTIO_VSOCK_OP_REQUEST) {
            switch (static_cast<DstPort>(request->dst_port)) {
                case DstPort::Data: {
                    auto stream = createStreamLocked(request, nullptr, nullptr);
                    if (stream) {
                        stream->sendOp(VIRTIO_VSOCK_OP_RESPONSE);
                    } else {
                        derror("%s {src_port=%u dst_port=%u} could not create "
                               "the stream - already exists",
                               __func__, request->src_port, request->dst_port);
                        queueOrphanFrameHostToGuestLocked(request,
                                                          VIRTIO_VSOCK_OP_RST);
                    }
                } break;

                case DstPort::Ping:
                    for (const auto& kv : mStreams) {
                        kv.second->signalWake(false);
                    }

                    // this port is not intended to be open
                    queueOrphanFrameHostToGuestLocked(request,
                                                      VIRTIO_VSOCK_OP_RST);
                    break;

                case DstPort::DataNewTransport: {
                    auto stream = createNewStreamLocked(request);
                    if (stream) {
                        stream->sendOp(VIRTIO_VSOCK_OP_RESPONSE);
                    } else {
                        derror("%s {src_port=%u dst_port=%u} could not create "
                               "the stream - already exists",
                               __func__, request->src_port, request->dst_port);
                        queueOrphanFrameHostToGuestLocked(request,
                                                          VIRTIO_VSOCK_OP_RST);
                    }
                } break;

                default:
                    derror("%s: {src_port=%u dst_port=%u} unexpected dst_port",
                           __func__, request->src_port, request->dst_port);

                    queueOrphanFrameHostToGuestLocked(request,
                                                      VIRTIO_VSOCK_OP_RST);
                    break;
            }
        } else {
            auto stream =
                    findStreamLocked(request->src_port, request->dst_port);
            if (stream) {
                switch (request->op) {
                    case VIRTIO_VSOCK_OP_SHUTDOWN:
                        stream->sendOp(VIRTIO_VSOCK_OP_SHUTDOWN);
                        closeStreamLocked(std::move(stream));
                        break;

                    case VIRTIO_VSOCK_OP_RST:
                        closeStreamLocked(std::move(stream));
                        break;

                    case VIRTIO_VSOCK_OP_RW:
                        stream->setGuestPos(request->buf_alloc,
                                            request->fwd_cnt);
                        if (stream->writeGuestToHost(request + 1,
                                                     request->len)) {
                            stream->sendOp(VIRTIO_VSOCK_OP_CREDIT_UPDATE);
                        } else {
                            stream->sendOp(VIRTIO_VSOCK_OP_CREDIT_UPDATE);
                            stream->sendOp(VIRTIO_VSOCK_OP_SHUTDOWN);
                            closeStreamLocked(std::move(stream));
                        }
                        break;

                    case VIRTIO_VSOCK_OP_RESPONSE:
                        stream->guestConnected();
                        /* fallthrough */

                    case VIRTIO_VSOCK_OP_CREDIT_UPDATE:
                        stream->setGuestPos(request->buf_alloc,
                                            request->fwd_cnt);
                        break;

                    case VIRTIO_VSOCK_OP_CREDIT_REQUEST:
                        stream->setGuestPos(request->buf_alloc,
                                            request->fwd_cnt);
                        stream->sendOp(VIRTIO_VSOCK_OP_CREDIT_UPDATE);
                        break;

                    default:
                        derror("%s unexpected op, op=%s (%d)", __func__,
                               op2str(request->op), request->op);

                        stream->sendOp(VIRTIO_VSOCK_OP_RST);
                        closeStreamLocked(std::move(stream));
                        break;
                }
            } else {
                switch (request->op) {
                    case VIRTIO_VSOCK_OP_RST:
                    case VIRTIO_VSOCK_OP_SHUTDOWN:
                        break;  // ignore those

                    default:
                        queueOrphanFrameHostToGuestLocked(request,
                                                          VIRTIO_VSOCK_OP_RST);
                        break;
                }
            }
        }
    }

    void vqParseGuestToHostLocked() {
        while (true) {
            if (mVqGuestToHostBuf.size() < sizeof(struct virtio_vsock_hdr)) {
                break;
            }

            const auto request =
                    reinterpret_cast<const struct virtio_vsock_hdr*>(
                            mVqGuestToHostBuf.data());
            const size_t requestSize = sizeof(*request) + request->len;
            if (mVqGuestToHostBuf.size() < requestSize) {
                break;
            }

            vqParseGuestToHostRequestLocked(request);

            if (requestSize == mVqGuestToHostBuf.size()) {
                mVqGuestToHostBuf.clear();
            } else {
                mVqGuestToHostBuf.erase(
                        mVqGuestToHostBuf.begin(),
                        mVqGuestToHostBuf.begin() + requestSize);
            }
        }
    }

    void vqReadGuestToHostLockedImpl(VirtQueueElement* e) {
        const size_t sz = iov_size(e->out_sg, e->out_num);

        const size_t currentSize = mVqGuestToHostBuf.size();
        mVqGuestToHostBuf.resize(currentSize + sz);
        iov_to_buf(e->out_sg, e->out_num, 0, &mVqGuestToHostBuf[currentSize],
                   sz);

        vqParseGuestToHostLocked();
    }

    void vqReadGuestToHostLocked(VirtIODevice* dev, VirtQueue* vq) {
        while (virtio_queue_ready(vq)) {
            VirtQueueElement* e = static_cast<VirtQueueElement*>(
                    virtqueue_pop(vq, sizeof(VirtQueueElement)));
            if (e) {
                vqReadGuestToHostLockedImpl(e);
                vqConsumeVirtQueueElement(vq, e, 0);
            } else {
                break;
            }
        }

        if (virtio_queue_ready(vq)) {
            virtio_notify(dev, vq);
        }
    }

    uint32_t getNextSrcHostPort() {
        const uint32_t r = mSrcHostPortI;
        mSrcHostPortI = std::max(r + 1, kSrcHostPortMin);
        return r;
    }

    VirtIOVSock* const mS;
    std::unordered_map<uint64_t, std::shared_ptr<VSockStream>> mStreams;
    std::vector<uint8_t> mVqGuestToHostBuf;
    std::deque<struct virtio_vsock_hdr> mHostToGuestOrphanFrames;
    uint32_t mSrcHostPortI = kSrcHostPortMin;
    const bool mSnaphotLoadFixed_b231345789;
};

void VSockStream::sendOp(enum virtio_vsock_op op) {
    switch (op) {
        case VIRTIO_VSOCK_OP_SHUTDOWN:
            if (mIsConnected) {
                struct virtio_vsock_hdr hdr =
                        prepareFrameHeader(this, VIRTIO_VSOCK_OP_SHUTDOWN, 0);
                hdr.flags =
                        VIRTIO_VSOCK_SHUTDOWN_RCV | VIRTIO_VSOCK_SHUTDOWN_SEND;
                mDev->queueOrphanFrameHostToGuestLocked(hdr);
            }
            break;

        case VIRTIO_VSOCK_OP_RST:
            mDev->queueOrphanFrameHostToGuestLocked(
                    prepareFrameHeader(this, VIRTIO_VSOCK_OP_RST, 0));
            break;

        default:
            mSendOpMask |= (1u << op);
            break;
    }
}

void VSockStream::signalWake(const bool write) {
    if (mPipe) {
        while (true) {
            uint8_t data[1024];
            AndroidPipeBuffer abuf;
            abuf.data = data;
            abuf.size = sizeof(data);

            const int n = android_pipe_guest_recv(mPipe, &abuf, 1);
            if (n > 0) {
                hostToGuestBufAppend(data, n);
            } else {
                break;
            }
        }
    } else if (mHostCallbacks) {
        // nothing
    } else {
        ::crashhandler_die("%s:%d: No data source", __func__, __LINE__);
    }
    if (write) {
        mDev->vqWriteHostToGuestExternalLocked();
    }
}

void VSockStream::closeFromHostCallback(void* that) {
    VSockStream* stream = static_cast<VSockStream*>(that);
    stream->mPipe = nullptr;  // TODO: already closed?
    stream->mDev->closeStreamFromHostLocked(stream);
}
}  // namespace

// the host to guest vq has space in it
void virtio_vsock_handle_host_to_guest(VirtIODevice* dev, VirtQueue* vq) {
    VirtIOVSock* s = VIRTIO_VSOCK(dev);
    static_cast<VirtIOVSockDev*>(s->impl)->vqWriteHostToGuestLocked();
}

// the guest to host vq has data in it
void virtio_vsock_handle_guest_to_host(VirtIODevice* dev, VirtQueue* vq) {
    VirtIOVSock* s = VIRTIO_VSOCK(dev);
    static_cast<VirtIOVSockDev*>(s->impl)->vqGuestToHostCallbackLocked(dev, vq);
}

void virtio_vsock_handle_event_to_guest(VirtIODevice* dev, VirtQueue* vq) {}

VirtIOVSockDev* g_impl = nullptr;

void virtio_vsock_ctor(VirtIOVSock* s, Error** errp) {
    VirtIOVSockDev* dev = new VirtIOVSockDev(s);
    s->impl = dev;
    g_impl = dev;
}

void virtio_vsock_dctor(VirtIOVSock* s) {
    g_impl = nullptr;
    delete static_cast<VirtIOVSockDev*>(s->impl);
    s->impl = nullptr;
}

void virtio_vsock_set_status(VirtIODevice* dev, uint8_t status) {
    VirtIOVSock* s = VIRTIO_VSOCK(dev);
    static_cast<VirtIOVSockDev*>(s->impl)->setStatus(status);
}

void virtio_vsock_impl_save(QEMUFile* f, const void* impl) {
    android::qemu::QemuFileStream qstream(f);
    AdbVsockPipe::Service::save(&qstream);
    static_cast<const VirtIOVSockDev*>(impl)->save(&qstream);
}

int virtio_vsock_impl_load(QEMUFile* f, void* impl) {
    android::qemu::QemuFileStream qstream(f);
    AdbVsockPipe::Service::load(&qstream);
    return static_cast<VirtIOVSockDev*>(impl)->load(&qstream) ? 0 : 1;
}

int virtio_vsock_is_enabled() {
    return fc::isEnabled(fc::VirtioVsockPipe);
}

uint64_t virtio_vsock_host_to_guest_open(uint32_t guest_port,
                                         IVsockHostCallbacks* callbacks) {
    return g_impl ? g_impl->hostToGuestOpen(guest_port, callbacks) : 0;
}

size_t virtio_vsock_host_to_guest_send(uint64_t handle,
                                       const void* data,
                                       size_t size) {
    return g_impl ? g_impl->hostToGuestSend(handle, data, size) : 0;
}

bool virtio_vsock_host_to_guest_ping(uint64_t handle) {
    return g_impl ? g_impl->hostToGuestPing(handle) : false;
}

bool virtio_vsock_host_to_guest_close(uint64_t handle) {
    return g_impl ? g_impl->hostToGuestClose(handle) : false;
}

namespace {
const virtio_vsock_device_ops_t virtio_vsock_device_host_ops = {
        .open = &virtio_vsock_host_to_guest_open,
        .send = &virtio_vsock_host_to_guest_send,
        .ping = &virtio_vsock_host_to_guest_ping,
        .close = &virtio_vsock_host_to_guest_close,
};
}  // namespace

const virtio_vsock_device_ops_t* virtio_vsock_device_get_host_ops() {
    return &virtio_vsock_device_host_ops;
}

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

extern "C" {
#include <qemu/compiler.h>
#include <qemu/osdep.h>
#include <qemu/typedefs.h>
#include <qemu/iov.h>
#include <qapi/error.h>
#include <standard-headers/linux/virtio_vsock.h>
#include <hw/virtio/virtio-vsock.h>
}  // extern "C"

#include <android/android-emu/android/emulation/android_pipe_base.h>
#include <android/android-emu/android/emulation/android_pipe_common.h>
#include <android/android-emu/android/emulation/android_pipe_device.h>
#include <android-qemu2-glue/base/files/QemuFileStream.h>

namespace {
constexpr uint32_t kHostBufAlloc = 1024 * 1024;

enum class DstPort {
    Data = 5555,
    Ping = 5556,
};

const char* op2str(int op) {
    switch (op) {
    case VIRTIO_VSOCK_OP_REQUEST:        return "REQUEST";
    case VIRTIO_VSOCK_OP_SHUTDOWN:       return "SHUTDOWN";
    case VIRTIO_VSOCK_OP_RST:            return "RST";
    case VIRTIO_VSOCK_OP_RESPONSE:       return "RESPONSE";
    case VIRTIO_VSOCK_OP_RW:             return "RW";
    case VIRTIO_VSOCK_OP_CREDIT_UPDATE:  return "CREDIT_UPDATE";
    case VIRTIO_VSOCK_OP_CREDIT_REQUEST: return "CREDIT_REQUEST";
    default:                             return "(bad)";
    }
}

uint64_t makeStreamKey(uint32_t srcPort, uint32_t dstPort) {
    return (uint64_t(srcPort) << 32) | dstPort;
}

::Stream* asCStream(android::base::Stream *stream) {
    return reinterpret_cast<::Stream*>(stream);
}

void saveVector(const std::vector<uint8_t> &v, android::base::Stream *stream) {
    stream->putBe32(v.size());
    stream->write(v.data(), v.size());
}

bool loadVector(std::vector<uint8_t> *v, android::base::Stream *stream) {
    v->resize(stream->getBe32());
    return stream->read(v->data(), v->size()) == v->size();
}

struct SocketBuffer {
    void append(const void *data, size_t size) {
        const uint8_t *data8 = static_cast<const uint8_t *>(data);
        mBuf.insert(mBuf.end(), data8, data8 + size);
    }

    std::pair<const void*, size_t> peek() const {
        return {mBuf.data(), mBuf.size()};
    }

    void consume(size_t size) {
        mBuf.erase(mBuf.begin(), mBuf.begin() + size);
    }

    void clear() {
        mBuf.clear();
    }

    void save(android::base::Stream *stream) const {
        saveVector(mBuf, stream);
    }

    bool load(android::base::Stream *stream) {
        return loadVector(&mBuf, stream);
    }

    std::vector<uint8_t> mBuf;
};

struct VirtIOVSockDev;

struct VSockStream {
    struct ForSnapshotLoad {};

    VSockStream(ForSnapshotLoad, struct VirtIOVSockDev *d) : mDev(d) {}

    VSockStream(struct VirtIOVSockDev *d,
                int id,
                uint64_t src_cid,
                uint64_t dst_cid,
                uint32_t src_port,
                uint32_t dst_port,
                uint32_t guest_buf_alloc,
                uint32_t guest_fwd_cnt)
        : mDev(d)
        , mPipe(android_pipe_guest_open_with_flags(this, ANDROID_PIPE_VIRTIO_VSOCK_BIT))
        , mSrcCid(src_cid)
        , mDstCid(dst_cid)
        , mSrcPort(src_port)
        , mDstPort(dst_port)
        , mId(id)
        , mGuestBufAlloc(guest_buf_alloc)
        , mGuestFwdCnt(guest_fwd_cnt) {}

    ~VSockStream() {
        if (mPipe) {
            android_pipe_guest_close(mPipe, PIPE_CLOSE_GRACEFUL);
        }
    }

    bool ok() const {
        return mPipe != nullptr;
    }

    void setGuestPos(uint32_t buf_alloc, uint32_t fwd_cnt) {
        mGuestBufAlloc = buf_alloc;
        mGuestFwdCnt = fwd_cnt;
    }

    void writeGuestToHost(const void *buf, size_t size) {
        AndroidPipeBuffer abuf;
        abuf.data = static_cast<uint8_t *>(const_cast<void *>(buf));
        abuf.size = size;

        android_pipe_guest_send(&mPipe, &abuf, 1);

        mHostFwdCnt += size;
    }

    void hostToGuestBufAppend(const void *data, size_t size) {
        mHostToGuestBuf.append(data, size);
    }

    std::pair<const void*, size_t> hostToGuestBufPeek() const {
        const size_t guestSpaceAvailable = mGuestBufAlloc - (mHostSentCnt - mGuestFwdCnt);
        const auto b = mHostToGuestBuf.peek();
        return {b.first, std::min(guestSpaceAvailable, b.second)};
    }

    void hostToGuestBufConsume(size_t size) {
        mHostToGuestBuf.consume(size);
        mHostSentCnt += size;
    }

    bool signalWake();

    void save(android::base::Stream *stream) const {
        stream->putBe32(mId);
        android_pipe_guest_save(mPipe, asCStream(stream));

        stream->putBe64(mSrcCid);
        stream->putBe64(mDstCid);
        stream->putBe32(mSrcPort);
        stream->putBe32(mDstPort);

        stream->putBe32(mGuestBufAlloc);
        stream->putBe32(mGuestFwdCnt);
        stream->putBe32(mHostSentCnt);
        stream->putBe32(mHostFwdCnt);

        mHostToGuestBuf.save(stream);
    }

    bool load(android::base::Stream *stream) {
        mId = stream->getBe32();
        char force_close = 0;
        mPipe = android_pipe_guest_load(asCStream(stream), this, &force_close);
        if (!mPipe || force_close) {
            return false;
        }

        mSrcCid = stream->getBe64();
        mDstCid = stream->getBe64();
        mSrcPort = stream->getBe32();
        mDstPort = stream->getBe32();

        mGuestBufAlloc = stream->getBe32();
        mGuestFwdCnt = stream->getBe32();
        mHostSentCnt = stream->getBe32();
        mHostFwdCnt = stream->getBe32();

        return mHostToGuestBuf.load(stream);
    }

    static void closeFromHostCallback(void* that);

    static void signalWakeCallback(void* that, unsigned flags) {
        static_cast<VSockStream*>(that)->signalWake();
    }

    static int getPipeIdCallback(void* that) {
        return static_cast<const VSockStream*>(that)->mId;
    }

    const AndroidPipeHwFuncs *const vtblPtr = &vtbl;
    struct VirtIOVSockDev *const mDev;
    void *mPipe = nullptr;
    uint64_t mSrcCid = 0;
    uint64_t mDstCid = 0;
    uint32_t mSrcPort = 0;
    uint32_t mDstPort = 0;
    int mId = -1;
    uint32_t mGuestBufAlloc = 0;  // guest's buffer size
    uint32_t mGuestFwdCnt = 0;    // how much the guest received
    uint32_t mHostSentCnt = 0;    // how much the host sent
    uint32_t mHostFwdCnt = 0;     // how much the host received
    SocketBuffer mHostToGuestBuf;

    static const AndroidPipeHwFuncs vtbl;
};

const AndroidPipeHwFuncs VSockStream::vtbl = {
    &closeFromHostCallback,
    &signalWakeCallback,
    &getPipeIdCallback,
};

struct VirtIOVSockDev {
    VirtIOVSockDev(VirtIOVSock *s) : mS(s) {}

    void closeStreamFromHost(VSockStream *streamWeak) {
        closeStreamLocked(streamWeak->mSrcPort, streamWeak->mDstPort);
    }

    bool sendRWHostToGuest(VSockStream *stream) {
        while (true) {
            const auto b = stream->hostToGuestBufPeek();
            if (b.second == 0) {
                return false;  // stream is exhausted
            }

            const struct virtio_vsock_hdr hdr = {
                .src_cid = stream->mDstCid,
                .dst_cid = stream->mSrcCid,
                .src_port = stream->mDstPort,
                .dst_port = stream->mSrcPort,
                .len = static_cast<uint32_t>(b.second),
                .type = VIRTIO_VSOCK_TYPE_STREAM,
                .op = static_cast<uint16_t>(VIRTIO_VSOCK_OP_RW),
                .flags = 0,
                .buf_alloc = kHostBufAlloc,
                .fwd_cnt = stream->mHostFwdCnt,
            };

            std::vector<uint8_t> packet(sizeof(hdr) + b.second);
            memcpy(&packet[0], &hdr, sizeof(hdr));
            memcpy(&packet[sizeof(hdr)], b.first, b.second);

            const bool isVqFull = vqWriteHostToGuest(packet.data(), packet.size());

            stream->hostToGuestBufConsume(b.second);

            if (isVqFull) {
                return true;
            }
        }
    }

    void vqRecvGuestToHost(VirtQueueElement *e) {
        const size_t sz = iov_size(e->out_sg, e->out_num);

        std::lock_guard<std::mutex> lock(mMtx);

        const size_t currentSize = mVqGuestToHostBuf.size();
        mVqGuestToHostBuf.resize(currentSize + sz);
        iov_to_buf(e->out_sg, e->out_num, 0, &mVqGuestToHostBuf[currentSize], sz);

        vqParseGuestToHost();
    }

    void vqHostToGuestCallback() {
        std::lock_guard<std::mutex> lock(mMtx);
        vqHostToGuestImpl();
    }

    void save(android::base::Stream *stream) const {
        stream->putBe32(mIdGenerator);
        stream->putBe32(mStreams.size());
        for (const auto &kv : mStreams) {
            kv.second->save(stream);
        }
        saveVector(mVqGuestToHostBuf, stream);
        mVqHostToGuestBuf.save(stream);
    }

    bool load(android::base::Stream *stream) {
        mIdGenerator = stream->getBe32();
        for (uint32_t n = stream->getBe32(); n > 0; --n) {
            auto vstream = std::make_shared<VSockStream>(VSockStream::ForSnapshotLoad(), this);
            if (!vstream->load(stream)) {
                return false;
            }

            const auto key = makeStreamKey(vstream->mSrcPort, vstream->mDstPort);
            if (!mStreams.insert({key, std::move(vstream)}).second) {
                return false;
            }
        }

        if (!loadVector(&mVqGuestToHostBuf, stream)) {
            return false;
        }

        return mVqHostToGuestBuf.load(stream);
    }

    void setStatus(const uint8_t status) {
        if ((status & VIRTIO_CONFIG_S_DRIVER_OK) == 0) {
            resetDevice();
        }
    }

private:
    void resetDevice() {
        std::lock_guard<std::mutex> lock(mMtx);

        mStreams.clear();
        mVqGuestToHostBuf.clear();
        mVqHostToGuestBuf.clear();
    }

    void vqHostToGuestImpl() {
        for (auto &kv : mStreams) {
            if (kv.second->signalWake()) {
                break;
            }
        }
    }

    std::shared_ptr<VSockStream> createStreamLocked(const struct virtio_vsock_hdr *request) {
        auto stream = std::make_shared<VSockStream>(this,
                                                    ++mIdGenerator,
                                                    request->src_cid,
                                                    request->dst_cid,
                                                    request->src_port,
                                                    request->dst_port,
                                                    request->buf_alloc,
                                                    request->fwd_cnt);
        if (stream->ok()) {
            const auto r = mStreams.insert({makeStreamKey(request->src_port,
                                                          request->dst_port), {}});
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

    std::shared_ptr<VSockStream> findStreamLocked(uint32_t srcPort, uint32_t dstPort) {
        const auto i = mStreams.find(makeStreamKey(srcPort, dstPort));
        if (i == mStreams.end()) {
            return nullptr;
        } else {
            return i->second;
        }
    }

    void closeStreamLocked(const uint32_t srcPort, const uint32_t dstPort) {
        if (mStreams.erase(makeStreamKey(srcPort, dstPort)) == 0) {
            fprintf(stderr, "%s:%d could not find the stream to close, {srcPort=%u dstPort=%u}\n",
                    __func__, __LINE__, srcPort, dstPort);
        }
    }

    void closeStreamLocked(std::shared_ptr<VSockStream> stream) {
        closeStreamLocked(stream->mSrcPort, stream->mDstPort);
    }

    size_t vqWriteHostToGuestImpl(const uint8_t *buf8, size_t size) {
        VirtQueue *vq = mS->host_to_guest_vq;
        size_t rem = size;

        while ((rem > 0) && virtio_queue_ready(vq)) {
            VirtQueueElement *e = static_cast<VirtQueueElement *>(
                virtqueue_pop(vq, sizeof(VirtQueueElement)));
            if (!e) {
                break;
            }

            const size_t sz = iov_from_buf(e->in_sg, e->in_num, 0, buf8, rem);
            virtqueue_push(vq, e, sz);
            g_free(e);

            buf8 += sz;
            rem -= sz;
        }

        if (virtio_queue_ready(vq)) {
            virtio_notify(&mS->parent, vq);
        }

        return size - rem;
    }

    bool vqWriteHostToGuest(const void *buf, size_t size) {
        bool isVqFull = false;

        while (true) {
            const auto b = mVqHostToGuestBuf.peek();
            if (b.second > 0) {
                // mVqHostToGuestBuf has data
                const size_t sz = vqWriteHostToGuestImpl(static_cast<const uint8_t *>(b.first),
                                                         b.second);
                if (sz > 0) {
                    mVqHostToGuestBuf.consume(sz);
                }
                if (sz < b.second) {
                    isVqFull = true;
                    break;
                }
            } else {
                break;  // mVqHostToGuestBuf is empty
            }
        }

        const uint8_t *buf8 = static_cast<const uint8_t *>(buf);
        if (!isVqFull && (size > 0)) {
            const size_t sz = vqWriteHostToGuestImpl(buf8, size);
            buf8 += sz;
            size -= sz;
        }
        if (size > 0) {
            mVqHostToGuestBuf.append(buf8, size);
            isVqFull = true;
        }

        return isVqFull;
    }

    void vqWriteOpHostToGuest(const struct virtio_vsock_hdr *request,
                              const enum virtio_vsock_op op,
                              const VSockStream *stream) {
        const uint32_t flags = (op == VIRTIO_VSOCK_OP_SHUTDOWN) ?
            (VIRTIO_VSOCK_SHUTDOWN_RCV | VIRTIO_VSOCK_SHUTDOWN_SEND) : 0;

        const struct virtio_vsock_hdr response = {
            .src_cid = request->dst_cid,
            .dst_cid = request->src_cid,
            .src_port = request->dst_port,
            .dst_port = request->src_port,
            .len = 0,
            .type = VIRTIO_VSOCK_TYPE_STREAM,
            .op = static_cast<uint16_t>(op),
            .flags = flags,
            .buf_alloc = (stream ? kHostBufAlloc : 0),
            .fwd_cnt = (stream ? stream->mHostFwdCnt : 0),
        };

        vqWriteHostToGuest(&response, sizeof(response));
    }

    void vqParseGuestToHostRequest(const struct virtio_vsock_hdr *request) {
        if (request->type != VIRTIO_VSOCK_TYPE_STREAM) {
            fprintf(stderr, "%s:%d bad request type (%d), src_port=%u dst_port=%u len=%u\n",
                    __func__, __LINE__, request->type, request->src_port, request->dst_port, request->len);
            return;
        }

        if (request->op == VIRTIO_VSOCK_OP_REQUEST) {
            switch (static_cast<DstPort>(request->dst_port)) {
            case DstPort::Data: {
                    auto stream = createStreamLocked(request);
                    if (stream) {
                        vqWriteOpHostToGuest(request, VIRTIO_VSOCK_OP_RESPONSE, &*stream);
                    } else {
                        fprintf(stderr, "%s:%d {src_port=%u dst_port=%u} could not create "
                                "the stream - already exists\n",
                                __func__, __LINE__, request->src_port, request->dst_port);
                        vqWriteOpHostToGuest(request, VIRTIO_VSOCK_OP_RST, nullptr);
                    }
                }
                break;

            case DstPort::Ping:
                vqHostToGuestImpl();
                vqWriteOpHostToGuest(request, VIRTIO_VSOCK_OP_RST, nullptr);
                break;

            default:
                fprintf(stderr, "%s:%d {src_port=%u dst_port=%u} unexpected dst_port\n",
                        __func__, __LINE__, request->src_port, request->dst_port);

                vqWriteOpHostToGuest(request, VIRTIO_VSOCK_OP_RST, nullptr);
                break;
            }
        } else {
            auto stream = findStreamLocked(request->src_port, request->dst_port);
            if (stream) {
                switch (request->op) {
                case VIRTIO_VSOCK_OP_SHUTDOWN:
                    vqWriteOpHostToGuest(request, VIRTIO_VSOCK_OP_SHUTDOWN, &*stream);
                    closeStreamLocked(std::move(stream));
                    break;

                case VIRTIO_VSOCK_OP_RST:
                    closeStreamLocked(std::move(stream));
                    break;

                case VIRTIO_VSOCK_OP_RW:
                    stream->setGuestPos(request->buf_alloc, request->fwd_cnt);
                    stream->writeGuestToHost(request + 1, request->len);
                    vqWriteOpHostToGuest(request, VIRTIO_VSOCK_OP_CREDIT_UPDATE, &*stream);
                    break;

                case VIRTIO_VSOCK_OP_CREDIT_UPDATE:
                    stream->setGuestPos(request->buf_alloc, request->fwd_cnt);
                    sendRWHostToGuest(&*stream);
                    break;

                case VIRTIO_VSOCK_OP_CREDIT_REQUEST:
                    stream->setGuestPos(request->buf_alloc, request->fwd_cnt);
                    vqWriteOpHostToGuest(request, VIRTIO_VSOCK_OP_CREDIT_UPDATE, &*stream);
                    break;

                default:
                    fprintf(stderr, "%s:%d unexpected op, op=%s (%d)\n",
                            __func__, __LINE__,
                            op2str(request->op), request->op);

                    vqWriteOpHostToGuest(request, VIRTIO_VSOCK_OP_RST, &*stream);
                    closeStreamLocked(std::move(stream));
                    break;
                }
            } else if (request->op != VIRTIO_VSOCK_OP_RST) {
                fprintf(stderr, "%s:%d {src_port=%u dst_port=%u} stream not found for op=%s (%d)\n",
                        __func__, __LINE__,
                        request->src_port, request->dst_port, op2str(request->op), request->op);

                vqWriteOpHostToGuest(request, VIRTIO_VSOCK_OP_RST, nullptr);
            }
        }
    }

    void vqParseGuestToHost() {
        while (true) {
            if (mVqGuestToHostBuf.size() < sizeof(struct virtio_vsock_hdr)) {
                break;
            }

            const auto request =
                reinterpret_cast<const struct virtio_vsock_hdr *>(mVqGuestToHostBuf.data());
            const size_t requestSize = sizeof(*request) + request->len;
            if (mVqGuestToHostBuf.size() < requestSize) {
                break;
            }

            vqParseGuestToHostRequest(request);

            if (requestSize == mVqGuestToHostBuf.size()) {
                mVqGuestToHostBuf.clear();
            } else {
                mVqGuestToHostBuf.erase(mVqGuestToHostBuf.begin(),
                                        mVqGuestToHostBuf.begin() + requestSize);
            }
        }
    }

    VirtIOVSock *const mS;

    int mIdGenerator = 0;
    std::unordered_map<uint64_t, std::shared_ptr<VSockStream>> mStreams;

    std::vector<uint8_t> mVqGuestToHostBuf;
    SocketBuffer mVqHostToGuestBuf;

    mutable std::mutex mMtx;
};

bool VSockStream::signalWake() {
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

    return mDev->sendRWHostToGuest(this);
}

void VSockStream::closeFromHostCallback(void* that) {
    VSockStream *stream = static_cast<VSockStream*>(that);
    stream->mPipe = nullptr; // TODO: already closed?
    stream->mDev->closeStreamFromHost(stream);
}
}  // namespace

void virtio_vsock_handle_host_to_guest(VirtIODevice *dev, VirtQueue *vq) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);
    static_cast<VirtIOVSockDev *>(s->impl)->vqHostToGuestCallback();
}

void virtio_vsock_handle_guest_to_host(VirtIODevice *dev, VirtQueue *vq) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);
    VirtIOVSockDev *impl = static_cast<VirtIOVSockDev *>(s->impl);

    while (true) {
        VirtQueueElement *e = static_cast<VirtQueueElement *>(
            virtqueue_pop(vq, sizeof(VirtQueueElement)));
        if (!e) {
            break;
        }

        impl->vqRecvGuestToHost(e);

        virtqueue_push(vq, e, 0);
        g_free(e);
    }

    virtio_notify(dev, vq);
}

void virtio_vsock_handle_event_to_guest(VirtIODevice *dev, VirtQueue *vq) {
}

void virtio_vsock_ctor(VirtIOVSock *s, Error **errp) {
    s->impl = new VirtIOVSockDev(s);
}

void virtio_vsock_dctor(VirtIOVSock *s) {
    delete static_cast<VirtIOVSockDev *>(s->impl);
    s->impl = nullptr;
}

void virtio_vsock_set_status(VirtIODevice *dev, uint8_t status) {
    VirtIOVSock *s = VIRTIO_VSOCK(dev);
    static_cast<VirtIOVSockDev *>(s->impl)->setStatus(status);
}

int virtio_vsock_impl_load(QEMUFile *f, VirtIOVSock *s) {
    android::qemu::QemuFileStream qstream(f);
    auto new_impl = std::make_unique<VirtIOVSockDev>(s);
    if (new_impl->load(&qstream)) {
        delete static_cast<VirtIOVSockDev *>(s->impl);
        s->impl = new_impl.release();
        return 0;
    } else {
        return 1;
    }
}

void virtio_vsock_impl_save(QEMUFile *f, const void *impl) {
    android::qemu::QemuFileStream qstream(f);
    static_cast<const VirtIOVSockDev *>(impl)->save(&qstream);
}

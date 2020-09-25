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

struct SocketBuffer {
    void append(const void *data, size_t size) {
        const uint8_t *data8 = static_cast<const uint8_t *>(data);
        buf.insert(buf.end(), data8, data8 + size);
    }

    std::pair<const void*, size_t> peek() const {
        return {buf.data(), buf.size()};
    }

    void consume(size_t size) {
        buf.erase(buf.begin(), buf.begin() + size);
    }

    std::vector<uint8_t> buf;
};

struct VirtIOVSockDev;

struct VSockStream {
    VSockStream(struct VirtIOVSockDev *d,
                int fd,
                uint64_t src_cid,
                uint64_t dst_cid,
                uint32_t src_port,
                uint32_t dst_port,
                uint32_t guest_buf_alloc,
                uint32_t guest_fwd_cnt);
    ~VSockStream();

    bool ok() const;
    void getHostPos(uint32_t *buf_alloc, uint32_t *fwd_cnt);
    void setGuestPos(uint32_t buf_alloc, uint32_t fwd_cnt);
    void writeGuestToHost(const void *buf, size_t size,
                          uint32_t *buf_alloc, uint32_t *fwd_cnt);
    void signalWake(unsigned flags);

    void hostToGuestBufAppend(const void *data, size_t size);
    std::pair<const void*, size_t> hostToGuestBufPeek() const;
    void hostToGuestBufConsume(size_t);

    static void closeFromHostCallback(void* that);
    static void signalWakeCallback(void* that, unsigned flags);
    static int getPipeIdCallback(void* that);

    const AndroidPipeHwFuncs *const vtblPtr;
    struct VirtIOVSockDev *const mDev;
    void *mPipe;
    const uint64_t mSrcCid;
    const uint64_t mDstCid;
    const uint32_t mSrcPort;
    const uint32_t mDstPort;
    const int mFd;
    uint32_t mGuestBufAlloc;
    uint32_t mGuestFwdCnt;
    uint32_t mHostSentCnt = 0;
    uint32_t mHostFwdCnt = 0;
    SocketBuffer mHostToGuestBuf;

    static const AndroidPipeHwFuncs vtbl;
};

const AndroidPipeHwFuncs VSockStream::vtbl = {
    &closeFromHostCallback,
    &signalWakeCallback,
    &getPipeIdCallback,
};

struct VirtIOVSockDev {
    VirtIOVSockDev(VirtIOVSock *s);
    ~VirtIOVSockDev();

    void closeStreamFromHost(VSockStream *stream);
    void sendHostToGuest(VSockStream *stream, const void *data, size_t size);

    void vqRecvGuestToHost(VirtQueueElement *e);
    void vqHostToGuestCallback();

private:
    std::shared_ptr<VSockStream> openStreamLocked(const struct virtio_vsock_hdr *request,
                                                  uint32_t *buf_alloc,
                                                  uint32_t *fwd_cnt);
    std::shared_ptr<VSockStream> findStreamLocked(uint32_t src_port, uint32_t dst_port);
    void closeStreamLocked(std::shared_ptr<VSockStream> stream);

    bool sendRWHostToGuest(VSockStream *stream);

    size_t vqWriteHostToGuestImpl(const uint8_t *buf8, size_t size);
    bool vqWriteHostToGuest(const void *buf, const size_t size);
    void vqWriteOpHostToGuest(const struct virtio_vsock_hdr *request,
                              enum virtio_vsock_op op,
                              uint32_t buf_alloc,
                              uint32_t fwd_cnt);

    void vqParseGuestToHostRequest(const struct virtio_vsock_hdr *request);
    void vqParseGuestToHost();

    VirtIOVSock *const mS;

    int mFdGenerator = 0;
    std::unordered_map<uint64_t, std::shared_ptr<VSockStream>> mStreams;

    std::vector<uint8_t> mVqGuestToHostBuf;
    SocketBuffer mVqHostToGuestBuf;

    mutable std::mutex mMtx;
};

namespace {
uint64_t makeStreamKey(uint32_t src_port, uint32_t dst_port) {
    return (uint64_t(dst_port) << 32) | src_port;
}
}  // namespace

VSockStream::VSockStream(struct VirtIOVSockDev *d,
                         const int fd,
                         uint64_t src_cid,
                         uint64_t dst_cid,
                         uint32_t src_port,
                         uint32_t dst_port,
                         uint32_t guest_buf_alloc,
                         uint32_t guest_fwd_cnt)
        : vtblPtr(&vtbl)
        , mDev(d)
        , mPipe(android_pipe_guest_open_with_flags(this, ANDROID_PIPE_VIRTIO_VSOCK_BIT))
        , mSrcCid(src_cid)
        , mDstCid(dst_cid)
        , mSrcPort(src_port)
        , mDstPort(dst_port)
        , mFd(fd)
        , mGuestBufAlloc(guest_buf_alloc)
        , mGuestFwdCnt(guest_fwd_cnt) {
    fprintf(stderr, "rkir555 %s:%d this=%p mDev=%p fd=%d mPipe=%p mSrcPort=%u mDstPort=%u\n",
            __func__, __LINE__, this, mDev, fd, mPipe, mSrcPort, mDstPort);
}

VSockStream::~VSockStream() {
    fprintf(stderr, "rkir555 %s:%d this=%p fd=%d mPipe=%p mSrcPort=%u mDstPort=%u\n",
            __func__, __LINE__, this, mFd, mPipe, mSrcPort, mDstPort);

    if (mPipe) {
        android_pipe_guest_close(mPipe, PIPE_CLOSE_GRACEFUL);
    }
}

bool VSockStream::ok() const {
    return mPipe != nullptr;
}

void VSockStream::getHostPos(uint32_t *buf_alloc, uint32_t *fwd_cnt) {
    *buf_alloc = 65536;
    *fwd_cnt = mHostFwdCnt;
}

void VSockStream::setGuestPos(uint32_t buf_alloc, uint32_t fwd_cnt) {
    mGuestBufAlloc = buf_alloc;
    mGuestFwdCnt = fwd_cnt;
}

void VSockStream::writeGuestToHost(const void *buf, size_t size,
                                   uint32_t *buf_alloc, uint32_t *fwd_cnt) {
    AndroidPipeBuffer abuf;
    abuf.data = static_cast<uint8_t *>(const_cast<void *>(buf));
    abuf.size = size;

    android_pipe_guest_send(&mPipe, &abuf, 1);

    mHostFwdCnt += size;
    getHostPos(buf_alloc, fwd_cnt);
}

void VSockStream::signalWake(unsigned flags) {
    fprintf(stderr, "rkir555 %s:%d this=%p flags=%x\n", __func__, __LINE__, this, flags);

    int n;
    uint8_t data[256];
    do {
        AndroidPipeBuffer abuf;
        abuf.data = data;
        abuf.size = sizeof(data);

        n = android_pipe_guest_recv(mPipe, &abuf, 1);
        if (n > 0) {
            mDev->sendHostToGuest(this, data, n);
        } else if (n < 0) {
            fprintf(stderr, "rkir555 %s:%d this=%p android_pipe_guest_recv failed with %d\n",
                    __func__, __LINE__, this, n);
        }
    } while (n == sizeof(data));
}

void VSockStream::hostToGuestBufAppend(const void *data, size_t size) {
    mHostToGuestBuf.append(data, size);
}

std::pair<const void*, size_t> VSockStream::hostToGuestBufPeek() const {
    const size_t guestSpaceAvailable = mGuestBufAlloc - (mHostSentCnt - mGuestFwdCnt);
    const auto b = mHostToGuestBuf.peek();
    return {b.first, std::min(guestSpaceAvailable, b.second)};
}

void VSockStream::hostToGuestBufConsume(size_t size) {
    mHostToGuestBuf.consume(size);
    mHostSentCnt += size;
}

void VSockStream::closeFromHostCallback(void* that) {
    VSockStream *stream = static_cast<VSockStream*>(that);
    stream->mDev->closeStreamFromHost(stream);
}

void VSockStream::signalWakeCallback(void* that, unsigned flags) {
    static_cast<VSockStream*>(that)->signalWake(flags);
}

int VSockStream::getPipeIdCallback(void* that) {
    return static_cast<const VSockStream*>(that)->mFd;
}

VirtIOVSockDev::VirtIOVSockDev(VirtIOVSock *s) : mS(s) {
}

VirtIOVSockDev::~VirtIOVSockDev() {
}

void VirtIOVSockDev::closeStreamFromHost(VSockStream* stream) {
    fprintf(stderr, "rkir555 %s:%d this=%p stream=%p\n",
            __func__, __LINE__, this, stream);

    std::lock_guard<std::mutex> lock(mMtx);
    // TODO
}

void VirtIOVSockDev::sendHostToGuest(VSockStream *stream,
                                     const void *data, size_t size) {
    fprintf(stderr, "rkir555 %s:%d this=%p stream=%p size=%zu\n",
            __func__, __LINE__, this, stream, size);

//    std::lock_guard<std::mutex> lock(mMtx);

//    fprintf(stderr, "rkir555 %s:%d this=%p stream=%p size=%zu\n",
//            __func__, __LINE__, this, stream, size);


    stream->hostToGuestBufAppend(data, size);
    sendRWHostToGuest(stream);
}

std::shared_ptr<VSockStream>
VirtIOVSockDev::openStreamLocked(const struct virtio_vsock_hdr *request,
                                 uint32_t *buf_alloc,
                                 uint32_t *fwd_cnt) {
    auto stream = std::make_shared<VSockStream>(this,
                                                ++mFdGenerator,
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
            stream->getHostPos(buf_alloc, fwd_cnt);
            r.first->second = stream;
            return stream;
        } else {
            fprintf(stderr, "rkir555 %s:%d failed src_port=%u dst_port=%u\n",
                    __func__, __LINE__, request->src_port, request->dst_port);

            return nullptr;
        }
    } else {
        fprintf(stderr, "rkir555 %s:%d failed src_port=%u dst_port=%u\n",
                __func__, __LINE__, request->src_port, request->dst_port);

        return nullptr;
    }
}

std::shared_ptr<VSockStream>
VirtIOVSockDev::findStreamLocked(uint32_t src_port, uint32_t dst_port) {
    const auto i = mStreams.find(makeStreamKey(src_port, dst_port));
    if (i == mStreams.end()) {
        fprintf(stderr, "rkir555 %s:%d failed src_port=%u dst_port=%u\n",
                __func__, __LINE__, src_port, dst_port);

        return nullptr;
    } else {
        return i->second;
    }
}

void VirtIOVSockDev::closeStreamLocked(std::shared_ptr<VSockStream> stream) {
    fprintf(stderr, "rkir555 %s:%d src_port=%u dst_port=%u\n",
            __func__, __LINE__, stream->mSrcPort, stream->mDstPort);

    mStreams.erase(makeStreamKey(stream->mSrcPort, stream->mDstPort));
}

size_t VirtIOVSockDev::vqWriteHostToGuestImpl(const uint8_t *buf8,
                                              const size_t size) {
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

    virtio_notify(&mS->parent, vq);
    return size - rem;
}

bool VirtIOVSockDev::vqWriteHostToGuest(const void *buf, size_t size) {
    const uint8_t *buf8 = static_cast<const uint8_t *>(buf);

    while (true) {
        const auto b = mVqHostToGuestBuf.peek();
        if (b.second == 0) {
            if (size > 0) {
                const size_t sz = vqWriteHostToGuestImpl(buf8, size);
                buf8 += sz;
                size -= sz;
            }
            break;
        }

        const size_t sz = vqWriteHostToGuestImpl(static_cast<const uint8_t *>(b.first),
                                                 b.second);
        if (sz > 0) {
            mVqHostToGuestBuf.consume(sz);
        }
        if (sz < b.second) {
            break;
        }
    }

    if (size > 0) {
        mVqHostToGuestBuf.append(buf8, size);
        return true;    // vq is full
    } else {
        return false;
    }
}

bool VirtIOVSockDev::sendRWHostToGuest(VSockStream *stream) {
    while (true) {
        const auto b = stream->hostToGuestBufPeek();
        if (b.second == 0) {
            return false;
        }

        uint32_t buf_alloc;
        uint32_t fwd_cnt;
        stream->getHostPos(&buf_alloc, &fwd_cnt);

        const struct virtio_vsock_hdr hdr = {
            .src_cid = stream->mDstCid,
            .dst_cid = stream->mSrcCid,
            .src_port = stream->mDstPort,
            .dst_port = stream->mSrcPort,
            .len = static_cast<uint32_t>(b.second),
            .type = VIRTIO_VSOCK_TYPE_STREAM,
            .op = static_cast<uint16_t>(VIRTIO_VSOCK_OP_RW),
            .flags = 0,
            .buf_alloc = buf_alloc,
            .fwd_cnt = fwd_cnt,
        };

        fprintf(stderr, "rkir555 %s:%d strem=%p src_cid=%u dst_cid=%u src_port=%u dst_port=%u op=%s len=%u buf_alloc=%u fwd_cnt=%u\n",
                __func__, __LINE__,
                stream,
                (uint32_t)hdr.src_cid, (uint32_t)hdr.dst_cid,
                hdr.src_port, hdr.dst_port,
                op2str(hdr.op), hdr.len,
                hdr.buf_alloc, hdr.fwd_cnt);

        std::vector<uint8_t> packet(sizeof(hdr) + b.second);
        memcpy(&packet[0], &hdr, sizeof(hdr));
        memcpy(&packet[sizeof(hdr)], b.first, b.second);

        const bool isVqFull = vqWriteHostToGuest(packet.data(), packet.size());
//        const bool isVqFull = vqWriteHostToGuest(b.first, b.second);

        stream->hostToGuestBufConsume(b.second);

        if (isVqFull) {
            return true;
        }
    }
}

void VirtIOVSockDev::vqWriteOpHostToGuest(
        const struct virtio_vsock_hdr *request,
        enum virtio_vsock_op op,
        uint32_t buf_alloc,
        uint32_t fwd_cnt) {
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
        .buf_alloc = buf_alloc,
        .fwd_cnt = fwd_cnt,
    };

    fprintf(stderr, "rkir555 %s:%d src_cid=%u dst_cid=%u src_port=%u dst_port=%u op=%s buf_alloc=%u fwd_cnt=%u\n",
            __func__, __LINE__,
            (uint32_t)response.src_cid, (uint32_t)response.dst_cid,
            response.src_port, response.dst_port,
            op2str(response.op),
            response.buf_alloc, response.fwd_cnt);

    vqWriteHostToGuest(&response, sizeof(response));
}

void VirtIOVSockDev::vqParseGuestToHostRequest(const struct virtio_vsock_hdr *request) {
    uint32_t buf_alloc;
    uint32_t fwd_cnt;

    if (request->type != VIRTIO_VSOCK_TYPE_STREAM) {
        fprintf(stderr, "rkir555 %s:%d this=%p bad request type\n", __func__, __LINE__, this);
        vqWriteOpHostToGuest(request, VIRTIO_VSOCK_OP_RST, 0, 0);
        return;
    }

    fprintf(stderr, "rkir555 %s:%d "
            "src_cid=%u dst_cid=%u "
            "src_port=%u dst_port=%u "
            "len=%u op=%s flags=0x%x "
            "buf_alloc=%u fwd_cnt=%u\n",
            __func__, __LINE__,
            (uint32_t)request->src_cid, (uint32_t)request->dst_cid,
            request->src_port, request->dst_port,
            request->len, op2str(request->op), request->flags,
            request->buf_alloc, request->fwd_cnt);

    if (request->op == VIRTIO_VSOCK_OP_REQUEST) {
        auto stream = openStreamLocked(request, &buf_alloc, &fwd_cnt);
        if (stream) {
            vqWriteOpHostToGuest(request, VIRTIO_VSOCK_OP_RESPONSE, buf_alloc, fwd_cnt);
        } else {
            fprintf(stderr, "rkir555 %s:%d this=%p could not open stream\n", __func__, __LINE__, this);
            vqWriteOpHostToGuest(request, VIRTIO_VSOCK_OP_RST, 0, 0);
        }
    } else {
        auto stream = findStreamLocked(request->src_port, request->dst_port);
        if (stream) {
            switch (request->op) {
            case VIRTIO_VSOCK_OP_SHUTDOWN:
                stream->getHostPos(&buf_alloc, &fwd_cnt);
                closeStreamLocked(std::move(stream));
                vqWriteOpHostToGuest(request, VIRTIO_VSOCK_OP_SHUTDOWN, buf_alloc, fwd_cnt);
                break;

            case VIRTIO_VSOCK_OP_RST:
                closeStreamLocked(std::move(stream));
                break;

            case VIRTIO_VSOCK_OP_RESPONSE:
                stream->getHostPos(&buf_alloc, &fwd_cnt);
                closeStreamLocked(std::move(stream));
                vqWriteOpHostToGuest(request, VIRTIO_VSOCK_OP_RST, buf_alloc, fwd_cnt);
                break;

            case VIRTIO_VSOCK_OP_RW:
                stream->setGuestPos(request->buf_alloc, request->fwd_cnt);
                stream->writeGuestToHost(request + 1, request->len,
                                         &buf_alloc, &fwd_cnt);
//                vqWriteOpHostToGuest(request, VIRTIO_VSOCK_OP_CREDIT_UPDATE, buf_alloc, fwd_cnt);
                break;

            case VIRTIO_VSOCK_OP_CREDIT_UPDATE:
                stream->setGuestPos(request->buf_alloc, request->fwd_cnt);
                sendRWHostToGuest(&*stream);
                break;

            case VIRTIO_VSOCK_OP_CREDIT_REQUEST:
                stream->setGuestPos(request->buf_alloc, request->fwd_cnt);
                stream->getHostPos(&buf_alloc, &fwd_cnt);
                vqWriteOpHostToGuest(request, VIRTIO_VSOCK_OP_CREDIT_UPDATE, buf_alloc, fwd_cnt);
                break;

            default:
                fprintf(stderr, "rkir555 %s:%d this=%p unexpected op, op=%d\n",
                        __func__, __LINE__, this, request->op);
                closeStreamLocked(std::move(stream));
                vqWriteOpHostToGuest(request, VIRTIO_VSOCK_OP_RST, 0, 0);
                break;
            }
        } else {
            fprintf(stderr, "rkir555 %s:%d this=%p stream {src_port=%u dst_port=%u} not found\n",
                    __func__, __LINE__, this, request->src_port, request->dst_port);
            vqWriteOpHostToGuest(request, VIRTIO_VSOCK_OP_RST, 0, 0);
        }
    }
}

void VirtIOVSockDev::vqParseGuestToHost() {
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

        mVqGuestToHostBuf.erase(mVqGuestToHostBuf.begin(),
                                mVqGuestToHostBuf.begin() + requestSize);
    }
}

void VirtIOVSockDev::vqHostToGuestCallback() {
    fprintf(stderr, "rkir555 %s:%d\n", __func__, __LINE__);

    std::lock_guard<std::mutex> lock(mMtx);
    fprintf(stderr, "rkir555 %s:%d mStreams.size()=%zu\n",
            __func__, __LINE__, mStreams.size());

    for (auto &kv : mStreams) {
        if (sendRWHostToGuest(&*kv.second)) {
            break;
        }
    }
}

void VirtIOVSockDev::vqRecvGuestToHost(VirtQueueElement *e) {
    const size_t sz = iov_size(e->out_sg, e->out_num);

    std::lock_guard<std::mutex> lock(mMtx);

    const size_t currentSize = mVqGuestToHostBuf.size();
    mVqGuestToHostBuf.resize(currentSize + sz);
    iov_to_buf(e->out_sg, e->out_num, 0, &mVqGuestToHostBuf[currentSize], sz);

    vqParseGuestToHost();
}

void virtio_vsock_handle_host_to_guest(VirtIODevice *dev, VirtQueue *vq) {
    fprintf(stderr, "rkir555 %s:%d dev=%p vq=%p\n", __func__, __LINE__, dev, vq);

    VirtIOVSock *s = VIRTIO_VSOCK(dev);
    VirtIOVSockDev *impl = static_cast<VirtIOVSockDev *>(s->impl);
    impl->vqHostToGuestCallback();
}

void virtio_vsock_handle_guest_to_host(VirtIODevice *dev, VirtQueue *vq) {
    fprintf(stderr, "rkir555 %s:%d dev=%p vq=%p\n", __func__, __LINE__, dev, vq);

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
    fprintf(stderr, "rkir555 %s:%d dev=%p vq=%p\n", __func__, __LINE__, dev, vq);
}

void virtio_vsock_ctor(VirtIOVSock *s, Error **errp) {
    s->impl = new VirtIOVSockDev(s);
}

void virtio_vsock_dctor(VirtIOVSock *s) {
    delete static_cast<VirtIOVSockDev *>(s->impl);
    s->impl = nullptr;
}

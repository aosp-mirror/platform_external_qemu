#include <atomic>
#include <vector>
#include <unordered_map>
#include <memory>

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

struct VirtIOVSockDev;

struct VSockStream {
    VSockStream(struct VirtIOVSockDev *d,
                int fd,
                uint32_t src_port,
                uint32_t dst_port,
                uint32_t guest_buf_alloc,
                uint32_t guest_fwd_cnt);
    ~VSockStream();

    const AndroidPipeHwFuncs *const vtblPtr;
    struct VirtIOVSockDev *mDev;
    void *mPipe;
    int mFd;
    uint32_t mSrcPort;
    uint32_t mDstPort;
    uint32_t mGuestBufAlloc;
    uint32_t mGuestFwdCnt;
    uint32_t mHostSentCnt = 0;
    uint32_t mHostFwdCnt = 0;
    std::vector<uint8_t> mHostBuf;

    bool ok() const;
    void getHostPos(uint32_t *buf_alloc, uint32_t *fwd_cnt);
    void setGuestPos(uint32_t buf_alloc, uint32_t fwd_cnt);
    void writeGuestToHost(const void *buf, size_t size,
                          uint32_t *buf_alloc, uint32_t *fwd_cnt);

    static void closeFromHostCallback(void* that);
    static void signalWakeCallback(void* that, unsigned flags);
    static int getPipeIdCallback(void* that);

    static const AndroidPipeHwFuncs vtbl;
};

const AndroidPipeHwFuncs VSockStream::vtbl = {
    &closeFromHostCallback,
    &signalWakeCallback,
    &getPipeIdCallback,
};

struct VirtIOVSockDev {
    VirtIOVSockDev();
    ~VirtIOVSockDev();

    std::shared_ptr<VSockStream> openStream(uint32_t src_port,
                                            uint32_t dst_port,
                                            uint32_t remote_buf_alloc,
                                            uint32_t remote_fwd_cnt,
                                            uint32_t *buf_alloc,
                                            uint32_t *fwd_cnt);
    std::shared_ptr<VSockStream> findStream(uint32_t src_port, uint32_t dst_port);
    void closeStream(std::shared_ptr<VSockStream> stream);

    size_t writeHostToGuestImpl(VirtQueue *vq,
                                const uint8_t *buf8,
                                const size_t size);
    void writeHostToGuest(const void *buf, const size_t size);
    void writeOpHostToGuest(
        const struct virtio_vsock_hdr *request,
        enum virtio_vsock_op op,
        uint32_t buf_alloc,
        uint32_t fwd_cnt);

    void parseRequest(const struct virtio_vsock_hdr *request);
    void parse();
    void *recvGuestToHost(size_t sz);

    std::atomic<int> mFdGenerator = 0;
    std::unordered_map<uint64_t, std::shared_ptr<VSockStream>> mStreams;
    std::vector<uint8_t> mGuestToHostBuf;
    std::vector<uint8_t> mHostToGuestBuf;  // TODO: this one should be smarter
};

namespace {
VirtIOVSock *gVsockHw = nullptr;

uint64_t makeStreamKey(uint32_t src_port, uint32_t dst_port) {
    return (uint64_t(dst_port) << 32) | src_port;
}
}  // namespace

VSockStream::VSockStream(struct VirtIOVSockDev *d,
                         const int fd,
                         uint32_t src_port,
                         uint32_t dst_port,
                         uint32_t guest_buf_alloc,
                         uint32_t guest_fwd_cnt)
        : vtblPtr(&vtbl)
        , mDev(d)
        , mPipe(android_pipe_guest_open_with_flags(this, ANDROID_PIPE_VIRTIO_VSOCK_BIT))
        , mFd(fd)
        , mSrcPort(src_port)
        , mDstPort(dst_port)
        , mGuestBufAlloc(guest_buf_alloc)
        , mGuestFwdCnt(guest_fwd_cnt) {
    mHostBuf.resize(64 * 1024);
    fprintf(stderr, "rkir555 %s:%d this=%p mDev=%p fd=%d mPipe=%p mSrcPort=%u mDstPort=%u\n",
            __func__, __LINE__, this, mDev, fd, mPipe, mSrcPort, mDstPort);
}

VSockStream::~VSockStream() {
    fprintf(stderr, "rkir555 %s:%d this=%p fd=%d mPipe=%p\n", __func__, __LINE__, this, mFd, mPipe);

    if (mPipe) {
        android_pipe_guest_close(mPipe, PIPE_CLOSE_GRACEFUL);
    }
}

bool VSockStream::ok() const {
    fprintf(stderr, "rkir555 %s:%d this=%p mPipe=%p\n",
            __func__, __LINE__, this, mPipe);

    return mPipe != nullptr;
}

void VSockStream::getHostPos(uint32_t *buf_alloc, uint32_t *fwd_cnt) {
    *buf_alloc = mHostBuf.size();
    *fwd_cnt = mHostFwdCnt;

    fprintf(stderr, "rkir555 %s:%d this=%p buf_alloc=%u fwd_cnt=%u\n",
            __func__, __LINE__, this, *buf_alloc, *fwd_cnt);
}

void VSockStream::setGuestPos(uint32_t buf_alloc, uint32_t fwd_cnt) {
    fprintf(stderr, "rkir555 %s:%d this=%p buf_alloc=%u fwd_cnt=%u\n",
            __func__, __LINE__, this, buf_alloc, fwd_cnt);

    mGuestBufAlloc = buf_alloc;
    mGuestFwdCnt = fwd_cnt;
}

void VSockStream::writeGuestToHost(const void *buf, size_t size,
                                   uint32_t *buf_alloc, uint32_t *fwd_cnt) {
    fprintf(stderr, "rkir555 %s:%d this=%p buf=%p size=%zu mPipe=%p\n",
            __func__, __LINE__, this, buf, size, mPipe);

    AndroidPipeBuffer abuf;
    abuf.data = static_cast<uint8_t *>(const_cast<void *>(buf));
    abuf.size = size;

    int r = android_pipe_guest_send(&mPipe, &abuf, 1);

    fprintf(stderr, "rkir555 %s:%d this=%p buf=%p size=%zu r=%d\n",
            __func__, __LINE__, this, buf, size, r);

    mHostFwdCnt += size;
    *buf_alloc = mHostBuf.size();
    *fwd_cnt = mHostFwdCnt;
}

void VSockStream::closeFromHostCallback(void* that) {
    fprintf(stderr, "rkir555 %s:%d that=%p\n", __func__, __LINE__, that);
}

void VSockStream::signalWakeCallback(void* that, unsigned flags) {
    fprintf(stderr, "rkir555 %s:%d that=%p\n", __func__, __LINE__, that);
}

int VSockStream::getPipeIdCallback(void* that) {
    fprintf(stderr, "rkir555 %s:%d that=%p\n", __func__, __LINE__, that);
    return static_cast<const VSockStream*>(that)->mFd;
}

VirtIOVSockDev::VirtIOVSockDev() {
    fprintf(stderr, "rkir555 %s:%d this=%p\n", __func__, __LINE__, this);
}


VirtIOVSockDev::~VirtIOVSockDev() {
    fprintf(stderr, "rkir555 %s:%d this=%p\n", __func__, __LINE__, this);
}

std::shared_ptr<VSockStream>
VirtIOVSockDev::openStream(uint32_t src_port,
                           uint32_t dst_port,
                           uint32_t guest_buf_alloc,
                           uint32_t guest_fwd_cnt,
                           uint32_t *buf_alloc,
                           uint32_t *fwd_cnt) {
    fprintf(stderr, "rkir555 %s:%d src_port=%u dst_port=%u\n",
            __func__, __LINE__, src_port, dst_port);

    auto stream = std::make_shared<VSockStream>(this,
                                                ++mFdGenerator,
                                                src_port,
                                                dst_port,
                                                guest_buf_alloc,
                                                guest_fwd_cnt);
    if (stream->ok()) {
        const auto r = mStreams.insert({makeStreamKey(src_port, dst_port), {}});
        if (r.second) {
            stream->getHostPos(buf_alloc, fwd_cnt);
            r.first->second = stream;
            return stream;
        } else {
            return nullptr;
        }
    } else {
        return nullptr;
    }
}

std::shared_ptr<VSockStream>
VirtIOVSockDev::findStream(uint32_t src_port, uint32_t dst_port) {
    fprintf(stderr, "rkir555 %s:%d src_port=%u dst_port=%u\n",
            __func__, __LINE__, src_port, dst_port);

    const auto i = mStreams.find(makeStreamKey(src_port, dst_port));
    if (i == mStreams.end()) {
        return nullptr;
    } else {
        return i->second;
    }
}

void VirtIOVSockDev::closeStream(std::shared_ptr<VSockStream> stream) {
    fprintf(stderr, "rkir555 %s:%d src_port=%u dst_port=%u\n",
            __func__, __LINE__, stream->mSrcPort, stream->mDstPort);

    mStreams.erase(makeStreamKey(stream->mSrcPort, stream->mDstPort));
}

size_t VirtIOVSockDev::writeHostToGuestImpl(VirtQueue *vq,
                                            const uint8_t *buf8,
                                            const size_t size) {
    fprintf(stderr, "rkir555 %s:%d buf8=%p size=%zu\n", __func__, __LINE__, buf8, size);

    size_t rem = size;

    while (rem > 0) {
        VirtQueueElement *e = static_cast<VirtQueueElement *>(
            virtqueue_pop(vq, sizeof(VirtQueueElement)));
        if (!e) {
            break;
        }

        size_t chunk_size = iov_from_buf(e->in_sg, e->in_num, 0, buf8, rem);
        virtqueue_push(vq, e, chunk_size);
        g_free(e);

        buf8 += chunk_size;
        rem -= chunk_size;
    }

    return size - rem;
}


void VirtIOVSockDev::writeHostToGuest(const void *buf, size_t size) {
    VirtQueue *vq = gVsockHw->host_to_guest_vq;
    size_t written = 0;

    if (!mHostToGuestBuf.empty()) {
        const size_t sz =
            writeHostToGuestImpl(vq,
                                 mHostToGuestBuf.data(),
                                 mHostToGuestBuf.size());
        if (sz > 0) {
            written += sz;
            mHostToGuestBuf.erase(mHostToGuestBuf.begin(),
                                  mHostToGuestBuf.begin() + sz);
        }
    }

    const uint8_t *buf8 = static_cast<const uint8_t *>(buf);

    if (mHostToGuestBuf.empty() && (size > 0)) {
        const size_t sz = writeHostToGuestImpl(vq, buf8, size);
        written += sz;
        buf8 += sz;
        size -= sz;
    }

    if (size > 0) {
        mHostToGuestBuf.insert(mHostToGuestBuf.end(), buf8, buf8 + size);
    }

    if (written > 0) {
        virtio_notify(&gVsockHw->parent, vq);
    }
}

void VirtIOVSockDev::writeOpHostToGuest(
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

    fprintf(stderr, "rkir555 %s:%d src_cid=%u dst_cid=%u src_port=%u dst_port=%u op=%d len=%u\n",
            __func__, __LINE__,
            (uint32_t)response.src_cid, (uint32_t)response.dst_cid,
            response.src_port, response.dst_port,
            response.op, response.len);

    writeHostToGuest(&response, sizeof(response));
}

void VirtIOVSockDev::parseRequest(const struct virtio_vsock_hdr *request) {
    uint32_t buf_alloc;
    uint32_t fwd_cnt;

    if (request->type != VIRTIO_VSOCK_TYPE_STREAM) {
        fprintf(stderr, "rkir555 %s:%d this=%p bad request type\n", __func__, __LINE__, this);
        writeOpHostToGuest(request, VIRTIO_VSOCK_OP_RST, 0, 0);
        return;
    }

    fprintf(stderr, "rkir555 %s:%d "
            "src_cid=%u dst_cid=%u "
            "src_port=%u dst_port=%u "
            "len=%u type=%u op=%u flags=0x%x "
            "buf_alloc=%u fwd_cnt=%u\n",
            __func__, __LINE__,
            (uint32_t)request->src_cid, (uint32_t)request->dst_cid,
            request->src_port, request->dst_port,
            request->len, request->type, request->op, request->flags,
            request->buf_alloc, request->fwd_cnt);

    if (request->op == VIRTIO_VSOCK_OP_REQUEST) {
        fprintf(stderr, "rkir555 %s:%d this=%p VIRTIO_VSOCK_OP_REQUEST\n", __func__, __LINE__, this);
        auto stream = openStream(request->src_port, request->dst_port,
                                 request->buf_alloc, request->fwd_cnt,
                                 &buf_alloc, &fwd_cnt);
        if (stream) {
            writeOpHostToGuest(request, VIRTIO_VSOCK_OP_RESPONSE, buf_alloc, fwd_cnt);
        } else {
            fprintf(stderr, "rkir555 %s:%d this=%p could not open stream\n", __func__, __LINE__, this);
            writeOpHostToGuest(request, VIRTIO_VSOCK_OP_RST, 0, 0);
        }
    } else {
        auto stream = findStream(request->src_port, request->dst_port);
        if (stream) {
            switch (request->op) {
            case VIRTIO_VSOCK_OP_SHUTDOWN:
                fprintf(stderr, "rkir555 %s:%d this=%p VIRTIO_VSOCK_OP_SHUTDOWN\n", __func__, __LINE__, this);
                stream->getHostPos(&buf_alloc, &fwd_cnt);
                closeStream(std::move(stream));
                writeOpHostToGuest(request, VIRTIO_VSOCK_OP_SHUTDOWN, buf_alloc, fwd_cnt);
                break;

            case VIRTIO_VSOCK_OP_RST:
                fprintf(stderr, "rkir555 %s:%d this=%p VIRTIO_VSOCK_OP_RST\n", __func__, __LINE__, this);
                closeStream(std::move(stream));
                break;

            case VIRTIO_VSOCK_OP_RESPONSE:
                fprintf(stderr, "rkir555 %s:%d this=%p VIRTIO_VSOCK_OP_RESPONSE\n", __func__, __LINE__, this);
                stream->getHostPos(&buf_alloc, &fwd_cnt);
                closeStream(std::move(stream));
                writeOpHostToGuest(request, VIRTIO_VSOCK_OP_RST, buf_alloc, fwd_cnt);
                break;

            case VIRTIO_VSOCK_OP_RW:
                fprintf(stderr, "rkir555 %s:%d this=%p VIRTIO_VSOCK_OP_RW\n", __func__, __LINE__, this);
                stream->setGuestPos(request->buf_alloc, request->fwd_cnt);
                stream->writeGuestToHost(request + 1, request->len,
                                         &buf_alloc, &fwd_cnt);
                writeOpHostToGuest(request, VIRTIO_VSOCK_OP_CREDIT_UPDATE, buf_alloc, fwd_cnt);
                break;

            case VIRTIO_VSOCK_OP_CREDIT_UPDATE:
                fprintf(stderr, "rkir555 %s:%d this=%p VIRTIO_VSOCK_OP_CREDIT_UPDATE\n", __func__, __LINE__, this);
                stream->setGuestPos(request->buf_alloc, request->fwd_cnt);
                break;

            case VIRTIO_VSOCK_OP_CREDIT_REQUEST:
                fprintf(stderr, "rkir555 %s:%d this=%p VIRTIO_VSOCK_OP_CREDIT_REQUEST\n", __func__, __LINE__, this);
                stream->setGuestPos(request->buf_alloc, request->fwd_cnt);
                stream->getHostPos(&buf_alloc, &fwd_cnt);
                writeOpHostToGuest(request, VIRTIO_VSOCK_OP_CREDIT_UPDATE, buf_alloc, fwd_cnt);
                break;

            default:
                fprintf(stderr, "rkir555 %s:%d this=%p unexpected op, op=%d\n",
                        __func__, __LINE__, this, request->op);
                closeStream(std::move(stream));
                writeOpHostToGuest(request, VIRTIO_VSOCK_OP_RST, 0, 0);
                break;
            }
        } else {
            fprintf(stderr, "rkir555 %s:%d this=%p stream not found\n", __func__, __LINE__, this);
            writeOpHostToGuest(request, VIRTIO_VSOCK_OP_RST, 0, 0);
        }
    }
}

void VirtIOVSockDev::parse() {
    while (true) {
        if (mGuestToHostBuf.size() < sizeof(struct virtio_vsock_hdr)) {
            break;
        }

        const auto request =
            reinterpret_cast<const struct virtio_vsock_hdr *>(mGuestToHostBuf.data());
        const size_t requestSize = sizeof(*request) + request->len;
        if (mGuestToHostBuf.size() < requestSize) {
            break;
        }

        parseRequest(request);

        mGuestToHostBuf.erase(mGuestToHostBuf.begin(),
                              mGuestToHostBuf.begin() + requestSize);
    }
}

void *VirtIOVSockDev::recvGuestToHost(size_t sz) {
    const size_t currentSize = mGuestToHostBuf.size();
    mGuestToHostBuf.resize(currentSize + sz);
    return &mGuestToHostBuf[currentSize];
}

void virtio_vsock_handle_host_to_guest(VirtIODevice *dev, VirtQueue *vq) {
    fprintf(stderr, "rkir555 %s:%d\n", __func__, __LINE__);

    VirtIOVSock *s = VIRTIO_VSOCK(dev);
    VirtIOVSockDev *impl = static_cast<VirtIOVSockDev *>(s->impl);

    fprintf(stderr, "rkir555 %s:%d s=%p impl=%p\n", __func__, __LINE__, s, impl);

    impl->writeHostToGuest(nullptr, 0);
}

void virtio_vsock_handle_guest_to_host(VirtIODevice *dev, VirtQueue *vq) {
    fprintf(stderr, "rkir555 %s:%d\n", __func__, __LINE__);

    VirtIOVSock *s = VIRTIO_VSOCK(dev);
    VirtIOVSockDev *impl = static_cast<VirtIOVSockDev *>(s->impl);

    while (true) {
        VirtQueueElement *e = static_cast<VirtQueueElement *>(
            virtqueue_pop(vq, sizeof(VirtQueueElement)));
        if (!e) {
            break;
        }

        const size_t sz = iov_size(e->out_sg, e->out_num);
        void *dst = impl->recvGuestToHost(sz);
        if (dst) {
            iov_to_buf(e->out_sg, e->out_num, 0, dst, sz);
            fprintf(stderr, "rkir555 %s:%d sz=%zu\n", __func__, __LINE__, sz);
            impl->parse();
        }

        virtqueue_push(vq, e, 0);
        g_free(e);
    }

    virtio_notify(dev, vq);
}

void virtio_vsock_handle_event_to_guest(VirtIODevice *dev, VirtQueue *vq) {
    fprintf(stderr, "rkir555 %s:%d dev=%p vq=%p\n", __func__, __LINE__, dev, vq);
}

void virtio_vsock_ctor(VirtIOVSock *s, Error **errp) {
    s->impl = new VirtIOVSockDev();
    gVsockHw = s;
    fprintf(stderr, "rkir555 %s:%d\n", __func__, __LINE__);
}

void virtio_vsock_dctor(VirtIOVSock *s) {
    fprintf(stderr, "rkir555 %s:%d\n", __func__, __LINE__);
    gVsockHw = nullptr;
    delete static_cast<VirtIOVSockDev *>(s->impl);
    s->impl = nullptr;
}

// Copyright 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "android/base/synchronization/Lock.h"
#include "android/base/memory/LazyInstance.h"
#include "android/emulation/android_pipe_common.h"

#include <deque>
#include <string>
#include <unordered_map>

extern "C" {
#include "qemu/osdep.h"
#include "standard-headers/drm/drm_fourcc.h"
#include "standard-headers/linux/virtio_gpu.h"

#include "hw/misc/goldfish_pipe.h"
#include "hw/virtio/virtio-goldfish-pipe.h"

#include "virgl_hw.h"
}  // extern "C"

#define DEBUG_VIRTIO_GOLDFISH_PIPE 0

#if DEBUG_VIRTIO_GOLDFISH_PIPE

#define VGPLOG(fmt,...) \
    fprintf(stderr, "%s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);

#else
#define VGPLOG(fmt,...)
#endif

// Virtio Goldfish Pipe: Overview-----------------------------------------------
//
// Virtio Goldfish Pipe is meant for running goldfish pipe services with a
// stock Linux kernel that is already capable of virtio-gpu. It runs DRM
// VIRTGPU ioctls on top of a custom implementation of virglrenderer on the
// host side that doesn't (directly) do any rendering, but instead talks to
// host-side pipe services.
//
// This is mainly used for graphics at the moment, though it's possible to run
// other pipe services over virtio-gpu as well. virtio-gpu is selected over
// other devices primarily because of the existence of an API (virglrenderer)
// that is already somewhat separate from virtio-gpu, and not needing to create
// a new virtio device to handle goldfish pipe.
//
// How it works is, existing virglrenderer API are remapped to perform pipe
// operations. First of all, pipe operations consist of the following:
//
// - open() / close(): Starts or stops an instance of a pipe service.
//
// - write(const void* buf, size_t len) / read(const void* buf, size_t len):
// Sends or receives data over the pipe. The first write() is the name of the
// pipe service. After the pipe service is determined, the host calls
// resetPipe() to replace the host-side pipe instance with an instance of the
// pipe service.
//
// - reset(void* initialPipe, void* actualPipe): the operation that replaces an
// initial pipe with an instance of a pipe service.
//
// Next, here's how the pipe operations map to virglrenderer commands:
//
// - open() -> virgl_renderer_context_create(),
//             virgl_renderer_resource_create(),
//             virgl_renderer_resource_attach_iov()
//
// The open() corresponds to a guest-side open of a rendernode, which triggers
// context creation. Each pipe corresponds 1:1 with a drm virtgpu context id.
// We also associate an R8 resource with each pipe as the backing data for
// write/read.
//
// - close() -> virgl_rendrerer_resource_unref(),
//              virgl_renderer_context_destroy()
//
// The close() corresponds to undoing the operations of open().
//
// - write() -> virgl_renderer_transfer_write_iov() OR
//              virgl_renderer_submit_cmd()
//
// Pipe write() operation corresponds to performing a TRANSFER_TO_HOST ioctl on
// the resource created alongside open(), OR an EXECBUFFER ioctl.
//
// - read() -> virgl_renderer_transfer_read_iov()
//
// Pipe read() operation corresponds to performing a TRANSFER_FROM_HOST ioctl on
// the resource created alongside open().
//
// A note on synchronization----------------------------------------------------
//
// Unlike goldfish-pipe which handles write/read/open/close on the vcpu thread
// that triggered the particular operation, virtio-gpu handles the
// corresponding virgl operations in a bottom half that is triggered off the
// vcpu thread on a timer. This means that in the guest, if we want to ensure
// that a particular operation such as TRANSFER_TO_HOST completed on the host,
// we need to call VIRTGPU_WAIT, which ends up polling fences here. This is why
// we insert a fence after every operation in this code.
//
// Details on transfer mechanism: mapping 2D transfer to 1D ones----------------
//
// Resource objects are typically 2D textures, while we're wanting to transmit
// 1D buffers to the pipe services on the host.  DRM VIRTGPU uses the concept
// of a 'box' to represent transfers that do not involve an entire resource
// object.  Each box has a x, y, width and height parameter to define the
// extent of the transfer for a 2D texture.  In our use case, we only use the x
// and width parameters. We've also created the resource with R8 format
// (byte-by-byte) with width equal to the total size of the transfer buffer we
// want (around 1 MB).
//
// The resource object itself is currently backed via plain guest RAM, which
// can be physically not-contiguous from the guest POV, and therefore
// corresponds to a possibly-long list of pointers and sizes (iov) on the host
// side. The sync_iovec_to_linear and sync_linear_to_iovec helper functions
// convert the list of pointers to one contiguous buffer on the host, at the
// cost of a copy.  (TODO: see if we can use host coherent memory to do away
// with the copy).
//
// We can see this abstraction in use via the implementation of
// transferWriteIov and transferReadIov below, which sync the iovec to/from a
// linear buffer if necessary, and then perform a corresponding pip operation
// based on the box parameter's x and width values.

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;

using VirglCtxId = uint32_t;
using VirglResId = uint32_t;

static constexpr int kPipeTryAgain = -2;

struct VirtioGpuCmd {
    uint32_t op;
    uint32_t cmdSize;
    unsigned char buf[0];
} __attribute__((packed));

struct PipeCtxEntry {
    VirglCtxId ctxId;
    GoldfishHostPipe* hostPipe;
    int fence;
};

struct PipeResEntry {
    virgl_renderer_resource_create_args args;
    iovec* iov;
    uint32_t numIovs;
    void* linear;
    size_t linearSize;
    GoldfishHostPipe* hostPipe;
    VirglCtxId ctxId;
};

static inline uint32_t align_up(uint32_t n, uint32_t a) {
    return ((n + a - 1) / a) * a;
}

static inline uint32_t virgl_format_to_bpp(uint32_t format) {
    uint32_t bpp = 4U;

    switch (format) {
        case VIRGL_FORMAT_R8_UNORM:
            bpp = 1U;
            break;
        case VIRGL_FORMAT_B5G6R5_UNORM:
            bpp = 2U;
            break;
        case VIRGL_FORMAT_B8G8R8A8_UNORM:
        case VIRGL_FORMAT_R8G8B8A8_UNORM:
        case VIRGL_FORMAT_R8G8B8X8_UNORM:
        default:
            bpp = 4U;
            break;
    }

    return bpp;
}

static int sync_linear_to_iovec(PipeResEntry* res, uint64_t offset, const virgl_box* box) {

    uint32_t bpp = virgl_format_to_bpp(res->args.format);

    VGPLOG("offset: 0x%llx box: %u %u %u %u res bpp %u size %u x %u iovs %u linearSize %zu",
            (unsigned long long)offset,
            box->x, box->y, box->w, box->h,
            bpp, res->args.width, res->args.height,
            res->numIovs,
            res->linearSize);

    if (box->x > res->args.width || box->y > res->args.height)
        return 0;
    if (box->w == 0U || box->h == 0U)
        return 0;
    uint32_t w = std::min(box->w, res->args.width - box->x);
    uint32_t h = std::min(box->h, res->args.height - box->y);
    uint32_t stride = align_up(res->args.width * bpp, 16U);
    offset += box->y * stride + box->x * bpp;
    // height - 1 in order to treat the (w * bpp) row specially
    // (i.e., the last row does not occupy the full stride)
    size_t length = (h - 1U) * stride + w * bpp;
    if (offset + length > res->linearSize)
        return EINVAL;

    const char* linear = static_cast<const char*>(res->linear);
    for (uint32_t i = 0, iovOffset = 0U; length && i < res->numIovs; i++) {
        if (iovOffset + res->iov[i].iov_len > offset) {
            char* iov_base = static_cast<char*>(res->iov[i].iov_base);
            size_t copyLength = std::min(length, res->iov[i].iov_len);
            memcpy(iov_base + offset - iovOffset, linear, copyLength);
            VGPLOG("first word of iovbase %p: 0x%x", iov_base, *(uint32_t*)iov_base);
            linear += copyLength;
            offset += copyLength;
            length -= copyLength;
        }
        iovOffset += res->iov[i].iov_len;
    }

    return 0;
}

static int sync_iovec_to_linear(PipeResEntry* res, uint64_t offset, const virgl_box* box) {
    uint32_t bpp = virgl_format_to_bpp(res->args.format);

    VGPLOG("offset: 0x%llx box: %u %u %u %u res bpp %u size %u x %u iovs %u linearSize %zu",
            (unsigned long long)offset,
            box->x, box->y, box->w, box->h,
            bpp, res->args.width, res->args.height,
            res->numIovs,
            res->linearSize);

    if (box->x > res->args.width || box->y > res->args.height)
        return 0;
    if (box->w == 0U || box->h == 0U)
        return 0;
    uint32_t w = std::min(box->w, res->args.width - box->x);
    uint32_t h = std::min(box->h, res->args.height - box->y);
    uint32_t stride = align_up(res->args.width * bpp, 16U);
    VGPLOG("stride: %u", stride);
    offset += box->y * stride + box->x * bpp;
    VGPLOG("offset: %u", (uint32_t)offset);
    // height - 1 in order to treat the (w * bpp) row specially
    // (i.e., the last row does not occupy the full stride)
    size_t length = (h - 1U) * stride + w * bpp;
    if (offset + length > res->linearSize)
        return EINVAL;

    char* linear = static_cast<char*>(res->linear);
    for (uint32_t i = 0, iovOffset = 0U; length && i < res->numIovs; i++) {
        VGPLOG("i: %u", i);
        if (iovOffset + res->iov[i].iov_len > offset) {
            const char* iov_base = static_cast<const char*>(res->iov[i].iov_base);
            VGPLOG("first word of iov base %p: 0x%x iovlen: %zu",
                    iov_base,
                    *(uint32_t*)iov_base,
                    (size_t)(res->iov[i].iov_len));
            size_t copyLength = std::min(length, res->iov[i].iov_len);
            VGPLOG("copylen: %zu off: %zu iovoff: %u. copy: %p -> %p", copyLength, offset, iovOffset,
                    iov_base + offset - iovOffset,
                    linear);
            memcpy(linear, iov_base + offset - iovOffset, copyLength);
            VGPLOG("first word of linear %p: 0x%x",
                    linear,
                    *(uint32_t*)linear);
            linear += copyLength;
            offset += copyLength;
            length -= copyLength;
        }
        iovOffset += res->iov[i].iov_len;
    }

    return 0;
}


class PipeVirglRenderer {
public:
    PipeVirglRenderer() = default;

    int init(void* cookie, int flags, const struct virgl_renderer_callbacks* callbacks) {
        VGPLOG("cookie: %p", cookie);
        mCookie = cookie;
        mVirglRendererCallbacks = *callbacks;
        return 0;
    }

    void resetPipe(GoldfishHwPipe* hwPipe, GoldfishHostPipe* hostPipe) {
        VGPLOG("Want to reset hwpipe %p to hostpipe %p", hwPipe, hostPipe);
        VirglCtxId asCtxId = (VirglCtxId)(uintptr_t)hwPipe;
        auto it = mContexts.find(asCtxId);
        if (it == mContexts.end()) {
            fprintf(stderr, "%s: fatal: pipe id %u not found\n", __func__, asCtxId);
            abort();
        }

        auto& entry = it->second;
        VGPLOG("ctxid: %u prev hostpipe: %p", asCtxId, entry.hostPipe);
        entry.hostPipe = hostPipe;
        VGPLOG("ctxid: %u next hostpipe: %p", asCtxId, entry.hostPipe);

        // Also update any resources associated with it
        auto resourcesIt = mContextResources.find(asCtxId);

        if (resourcesIt == mContextResources.end()) return;

        const auto& resIds = resourcesIt->second;

        for (auto resId : resIds) {
            auto resEntryIt = mResources.find(resId);
            if (resEntryIt == mResources.end()) {
                fprintf(stderr, "%s: fatal: res id %u entry not found\n", __func__, resId);
                abort();
            }

            auto& resEntry = resEntryIt->second;
            resEntry.hostPipe = hostPipe;
        }
    }

    int createContext(VirglCtxId handle, uint32_t nlen, const char* name) {
        AutoLock lock(mLock);
        VGPLOG("ctxid: %u len: %u name: %s", handle, nlen, name);
        auto ops = ensureAndGetServiceOps();
        auto hostPipe = ops->guest_open_with_flags(
            reinterpret_cast<GoldfishHwPipe*>(handle),
            0x1 /* is virtio */);

        if (!hostPipe) {
            fprintf(stderr, "%s: failed to create hw pipe!\n", __func__);
            return -1;
        }

        PipeCtxEntry res = {
            handle, // ctxId
            hostPipe, // hostPipe
            0, // fence
        };

        VGPLOG("initial host pipe for ctxid %u: %p", handle, hostPipe);
        mContexts[handle] = res;
        return 0;
    }

    int destroyContext(VirglCtxId handle) {
        AutoLock lock(mLock);
        VGPLOG("ctxid: %u", handle);

        auto it = mContexts.find(handle);
        if (it == mContexts.end()) {
            fprintf(stderr, "%s: could not find context handle %u\n", __func__, handle);
            return -1;
        }

        auto ops = ensureAndGetServiceOps();
        auto hostPipe = it->second.hostPipe;

        if (!hostPipe) {
            fprintf(stderr, "%s: 0 is not a valid hostpipe\n", __func__);
            return -1;
        }

        ops->guest_close(hostPipe, GOLDFISH_PIPE_CLOSE_GRACEFUL);

        return 0;
    }

    int write(VirglCtxId ctxId, void* buffer, int bytes) {
        VGPLOG("ctxid: %u buffer: %p bytes: %d", ctxId, buffer, bytes);

        if (!buffer) {
            fprintf(stderr, "%s: error: buffer null\n", __func__);
            return -1;
        }
        AutoLock lock(mLock);
        auto ops = ensureAndGetServiceOps();
        auto it = mContexts.find(ctxId);

        GoldfishPipeBuffer buf = {
            buffer, (size_t)bytes,
        };

        auto hostPipe = it->second.hostPipe;

        size_t writtenBytes = 0;
        size_t wantedBytes = bytes;

        while (writtenBytes < wantedBytes) {
            auto status = ops->guest_send(hostPipe, &buf, 1);

            if (status > 0) {
                writtenBytes += status;
            } else if (status != kPipeTryAgain) {
                return EIO;
            }
        }

        mLastSubmitCmdCtxExists = true;
        mLastSubmitCmdCtx = ctxId;
        return 0;
    }

    int createFence(int client_fence_id, uint32_t cmd_type) {
        AutoLock lock(mLock);
        VGPLOG("fenceid: %u cmdtype: %u", client_fence_id, cmd_type);
        mFenceDeque.push_back(client_fence_id);
        return 0;
    }

    void poll() {
        VGPLOG("start");
        AutoLock lock(mLock);
        for (auto fence : mFenceDeque) {
            VGPLOG("write fence: %u", fence);
            mVirglRendererCallbacks.write_fence(mCookie, fence);
        }
        mFenceDeque.clear();
        VGPLOG("end");
    }

    int createResource(
            struct virgl_renderer_resource_create_args *args,
            struct iovec *iov, uint32_t num_iovs) {

        VGPLOG("handle: %u. num iovs: %u", args->handle, num_iovs);

        PipeResEntry e;
        e.args = *args;
        e.linear = 0;
        e.hostPipe = 0;
        allocResource(e, iov, num_iovs);

        AutoLock lock(mLock);
        mResources[args->handle] = e;
        return 0;
    }

    void unrefResource(uint32_t toUnrefId) {
        AutoLock lock(mLock);
        VGPLOG("handle: %u", toUnrefId);

        auto it = mResources.find(toUnrefId);
        if (it == mResources.end()) return;

        auto contextsIt = mResourceContexts.find(toUnrefId);
        if (contextsIt != mResourceContexts.end()) {
            mResourceContexts.erase(contextsIt->first);
        }

        for (auto& ctxIdResources : mContextResources) {
            detachResourceLocked(ctxIdResources.first, toUnrefId);
        }

        auto& entry = it->second;

        if (entry.linear) {
            free(entry.linear);
            entry.linear = nullptr;
        }

        if (entry.iov) {
            free(entry.iov);
            entry.iov = nullptr;
            entry.numIovs = 0;
        }
    }

    int attachIov(int resId, iovec* iov, int num_iovs) {
        AutoLock lock(mLock);

        VGPLOG("resid: %d numiovs: %d", resId, num_iovs);

        auto it = mResources.find(resId);
        if (it == mResources.end()) return ENOENT;

        auto& entry = it->second;
        VGPLOG("res linear: %p", entry.linear);
        if (!entry.linear) allocResource(entry, iov, num_iovs);

        VGPLOG("done");
        return 0;
    }

    void detachIov(int resId, iovec** iov, int* num_iovs) {
        AutoLock lock(mLock);

        auto it = mResources.find(resId);
        if (it == mResources.end()) return;

        auto& entry = it->second;

        if (num_iovs) {
            *num_iovs = entry.numIovs;
            VGPLOG("resid: %d numIovs: %d", resId, *num_iovs);
        } else {
            VGPLOG("resid: %d numIovs: 0", resId);
        }

        entry.numIovs = 0;

        if (entry.iov) free(entry.iov);
        entry.iov = nullptr;

        if (iov) {
            *iov = entry.iov;
        }

        allocResource(entry, entry.iov, entry.numIovs);
        VGPLOG("done");
    }

    int transferReadIov(int resId, uint64_t offset, virgl_box* box) {
        AutoLock lock(mLock);

        VGPLOG("resid: %d offset: 0x%llx. box: %u %u %u %u", resId,
               (unsigned long long)offset,
               box->x,
               box->y,
               box->w,
               box->h);

        auto it = mResources.find(resId);
        if (it == mResources.end()) return EINVAL;

        auto& entry = it->second;

        // Do the pipe service op here, if there is an associated hostpipe.
        auto hostPipe = entry.hostPipe;
        if (!hostPipe) return -1;

        auto ops = ensureAndGetServiceOps();

        size_t readBytes = (size_t)box->x;
        size_t wantedBytes = readBytes + (size_t)box->w;

        while (readBytes < wantedBytes) {
            GoldfishPipeBuffer buf = {
                ((char*)entry.linear) + readBytes, wantedBytes - readBytes,
            };
            auto status = ops->guest_recv(hostPipe, &buf, 1);

            if (status > 0) {
                readBytes += status;
            } else if (status != kPipeTryAgain) {
                return EIO;
            }
        }

        VGPLOG("Linear first word: %d", *(int*)(entry.linear));

        auto syncRes = sync_linear_to_iovec(&entry, offset, box);
        mLastSubmitCmdCtxExists = true;
        mLastSubmitCmdCtx = entry.ctxId;

        VGPLOG("done");

        return syncRes;
    }

    int transferWriteIov(int resId, uint64_t offset, virgl_box* box) {
        AutoLock lock(mLock);
        VGPLOG("resid: %d offset: 0x%llx", resId,
               (unsigned long long)offset);
        auto it = mResources.find(resId);
        if (it == mResources.end()) return EINVAL;

        auto& entry = it->second;
        auto syncRes = sync_iovec_to_linear(&entry, offset, box);

        // Do the pipe service op here, if there is an associated hostpipe.
        auto hostPipe = entry.hostPipe;
        if (!hostPipe) return syncRes;

        VGPLOG("resid: %d offset: 0x%llx hostpipe: %p", resId,
               (unsigned long long)offset, hostPipe);

        auto ops = ensureAndGetServiceOps();

        size_t writtenBytes = 0;
        size_t wantedBytes = writtenBytes + (size_t)box->w;

        while (writtenBytes < wantedBytes) {
            GoldfishPipeBuffer buf = {
                ((char*)entry.linear) + writtenBytes, wantedBytes - writtenBytes,
            };
            auto status = ops->guest_send(hostPipe, &buf, 1);

            if (status > 0) {
                writtenBytes += status;
            } else if (status != kPipeTryAgain) {
                return EIO;
            }
        }

        mLastSubmitCmdCtxExists = true;
        mLastSubmitCmdCtx = entry.ctxId;

        VGPLOG("done");
        return syncRes;
    }

    void attachResource(uint32_t ctxId, uint32_t resId) {
        AutoLock lock(mLock);
        VGPLOG("ctxid: %u resid: %u", ctxId, resId);

        auto resourcesIt = mContextResources.find(ctxId);

        if (resourcesIt == mContextResources.end()) {
            std::vector<VirglResId> ids;
            ids.push_back(resId);
            mContextResources[ctxId] = ids;
        } else {
            auto& ids = resourcesIt->second;
            auto idIt = std::find(ids.begin(), ids.end(), resId);
            if (idIt == ids.end())
                ids.push_back(resId);
        }

        auto contextsIt = mResourceContexts.find(resId);

        if (contextsIt == mResourceContexts.end()) {
            std::vector<VirglCtxId> ids;
            ids.push_back(ctxId);
            mResourceContexts[resId] = ids;
        } else {
            auto& ids = contextsIt->second;
            auto idIt = std::find(ids.begin(), ids.end(), resId);
            if (idIt == ids.end())
                ids.push_back(ctxId);
        }

        // Associate the host pipe of the resource entry with the host pipe of
        // the context entry.  That is, the last context to call attachResource
        // wins if there is any conflict.
        auto ctxEntryIt = mContexts.find(ctxId); auto resEntryIt =
            mResources.find(resId);

        if (ctxEntryIt == mContexts.end() ||
            resEntryIt == mResources.end()) return;

        VGPLOG("hostPipe: %p", ctxEntryIt->second.hostPipe);
        resEntryIt->second.hostPipe = ctxEntryIt->second.hostPipe;
        resEntryIt->second.ctxId = ctxId;
    }

    void detachResource(uint32_t ctxId, uint32_t toUnrefId) {
        AutoLock lock(mLock);
        VGPLOG("ctxid: %u resid: %u", ctxId, toUnrefId);
        detachResourceLocked(ctxId, toUnrefId);
    }

    int getResourceInfo(uint32_t resId, struct virgl_renderer_resource_info *info) {
        VGPLOG("resid: %u", resId);
        if (!info)
            return EINVAL;

        AutoLock lock(mLock);
        auto it = mResources.find(resId);
        if (it == mResources.end())
            return ENOENT;

        auto& entry = it->second;

        uint32_t bpp = 4U;
        switch (entry.args.format) {
            case VIRGL_FORMAT_B8G8R8A8_UNORM:
                info->drm_fourcc = DRM_FORMAT_BGRA8888;
                break;
            case VIRGL_FORMAT_B5G6R5_UNORM:
                info->drm_fourcc = DRM_FORMAT_BGR565;
                bpp = 2U;
                break;
            case VIRGL_FORMAT_R8G8B8A8_UNORM:
                info->drm_fourcc = DRM_FORMAT_RGBA8888;
                break;
            case VIRGL_FORMAT_R8G8B8X8_UNORM:
                info->drm_fourcc = DRM_FORMAT_RGBX8888;
                break;
            default:
                return EINVAL;
        }

        info->stride = align_up(entry.args.width * bpp, 16U);
        info->virgl_format = entry.args.format;
        info->handle = entry.args.handle;
        info->height = entry.args.height;
        info->width = entry.args.width;
        info->depth = entry.args.depth;
        info->flags = entry.args.flags;
        info->tex_id = 0;
        return 0;
    }

private:
    void allocResource(PipeResEntry& entry, iovec* iov, int num_iovs) {
        VGPLOG("entry linear: %p", entry.linear);
        if (entry.linear) free(entry.linear);

        size_t linearSize = 0;
        for (uint32_t i = 0; i < num_iovs; ++i) {
            VGPLOG("iov base: %p", iov[i].iov_base);
            linearSize += iov[i].iov_len;
            VGPLOG("has iov of %zu. linearSize current: %zu",
                   iov[i].iov_len, linearSize);
        }
        VGPLOG("final linearSize: %zu", linearSize);

        void* linear = nullptr;

        if (linearSize) linear = malloc(linearSize);

        entry.iov = (iovec*)malloc(sizeof(*iov) * num_iovs);
        entry.numIovs = num_iovs;
        memcpy(entry.iov, iov, num_iovs * sizeof(*iov));
        entry.linear = linear;
        entry.linearSize = linearSize;

        virgl_box initbox;
        initbox.x = 0;
        initbox.y = 0;
        initbox.w = (uint32_t)linearSize;
        initbox.h = 1;
    }

    void detachResourceLocked(uint32_t ctxId, uint32_t toUnrefId) {
        VGPLOG("ctxid: %u resid: %u", ctxId, toUnrefId);

        auto it = mContextResources.find(ctxId);
        if (it == mContextResources.end()) return;

        std::vector<VirglResId> withoutRes;
        for (auto resId : it->second) {
            if (resId != toUnrefId) {
                withoutRes.push_back(resId);
            }
        }
        mContextResources[ctxId] = withoutRes;

        auto resIt = mResources.find(toUnrefId);
        if (resIt == mResources.end()) return;

        resIt->second.hostPipe = 0;
        resIt->second.ctxId = 0;
    }


    GoldfishHwPipe* createNewHwPipeId() {
        return reinterpret_cast<GoldfishHwPipe*>(mNextHwPipe++);
    }

    inline const GoldfishPipeServiceOps* ensureAndGetServiceOps() {
        if (mServiceOps) return mServiceOps;
        mServiceOps = goldfish_pipe_get_service_ops();
        return mServiceOps;
    }

    Lock mLock;

    void* mCookie = nullptr;
    int mFlags = 0;
    virgl_renderer_callbacks mVirglRendererCallbacks;

    uint32_t mNextHwPipe = 1;
    const GoldfishPipeServiceOps* mServiceOps = nullptr;

    std::unordered_map<VirglCtxId, PipeCtxEntry> mContexts;
    std::unordered_map<VirglResId, PipeResEntry> mResources;
    std::unordered_map<VirglCtxId, std::vector<VirglResId>> mContextResources;
    std::unordered_map<VirglResId, std::vector<VirglCtxId>> mResourceContexts;
    bool mLastSubmitCmdCtxExists = false;
    uint32_t mLastSubmitCmdCtx = 0;
    // Other fences that aren't related to the fence covering a pipe buffer
    // submission.
    std::deque<int> mFenceDeque;
};

static LazyInstance<PipeVirglRenderer> sRenderer = LAZY_INSTANCE_INIT;

extern "C" {

static int pipe_virgl_renderer_init(
    void *cookie, int flags, struct virgl_renderer_callbacks *cb) {
    sRenderer->init(cookie, flags, cb);
    return 0;
}

static void pipe_virgl_renderer_poll(void) {
    sRenderer->poll();
}

static void* pipe_virgl_renderer_get_cursor_data(
    uint32_t resource_id, uint32_t *width, uint32_t *height) {
    return 0;
}

static int pipe_virgl_renderer_resource_create(
    struct virgl_renderer_resource_create_args *args,
    struct iovec *iov, uint32_t num_iovs) {

    return sRenderer->createResource(args, iov, num_iovs);
}

static void pipe_virgl_renderer_resource_unref(uint32_t res_handle) {
    sRenderer->unrefResource(res_handle);
}

static int pipe_virgl_renderer_context_create(
    uint32_t handle, uint32_t nlen, const char *name) {
    return sRenderer->createContext(handle, nlen, name);
}

static void pipe_virgl_renderer_context_destroy(uint32_t handle) {
    sRenderer->destroyContext(handle);
}

static int pipe_virgl_renderer_submit_cmd(void *buffer,
                                          int ctx_id,
                                          int bytes) {
    return sRenderer->write(ctx_id, buffer, bytes);
}

static int pipe_virgl_renderer_transfer_read_iov(
    uint32_t handle, uint32_t ctx_id,
    uint32_t level, uint32_t stride,
    uint32_t layer_stride,
    struct virgl_box *box,
    uint64_t offset, struct iovec *iov,
    int iovec_cnt) {
    return sRenderer->transferReadIov(handle, offset, box);
}

static int pipe_virgl_renderer_transfer_write_iov(
    uint32_t handle,
    uint32_t ctx_id,
    int level,
    uint32_t stride,
    uint32_t layer_stride,
    struct virgl_box *box,
    uint64_t offset,
    struct iovec *iovec,
    unsigned int iovec_cnt) {
    return sRenderer->transferWriteIov(handle, offset, box);
}

// Not implemented
static void pipe_virgl_renderer_get_cap_set(uint32_t, uint32_t*, uint32_t*) { }
static void pipe_virgl_renderer_fill_caps(uint32_t, uint32_t, void *caps) { }

static int pipe_virgl_renderer_resource_attach_iov(
    int res_handle, struct iovec *iov,
    int num_iovs) {
    return sRenderer->attachIov(res_handle, iov, num_iovs);
}

static void pipe_virgl_renderer_resource_detach_iov(
    int res_handle, struct iovec **iov, int *num_iovs) {
    return sRenderer->detachIov(res_handle, iov, num_iovs);
}

static int pipe_virgl_renderer_create_fence(
    int client_fence_id, uint32_t cmd_type) {
    sRenderer->createFence(client_fence_id, cmd_type);
    return 0;
}

static void pipe_virgl_renderer_force_ctx_0(void) { }

static void pipe_virgl_renderer_ctx_attach_resource(
    int ctx_id, int res_handle) {
    sRenderer->attachResource(ctx_id, res_handle);
}

static void pipe_virgl_renderer_ctx_detach_resource(
    int ctx_id, int res_handle) {
    sRenderer->detachResource(ctx_id, res_handle);
}

static int pipe_virgl_renderer_resource_get_info(
    int res_handle,
    struct virgl_renderer_resource_info *info) {
    return sRenderer->getResourceInfo(res_handle, info);
}

#define VIRGLRENDERER_API_PIPE_STRUCT_DEF(api) pipe_##api,

static struct virgl_renderer_virtio_interface s_virtio_interface = {
    LIST_VIRGLRENDERER_API(VIRGLRENDERER_API_PIPE_STRUCT_DEF)
};

static void pipe_virgl_write_fence(void *opaque, uint32_t fence)
{
    VGPLOG("call");
    virgl_write_fence(opaque, fence);
}

static virgl_renderer_gl_context
pipe_virgl_create_context(void *opaque, int scanout_idx,
                     struct virgl_renderer_gl_ctx_param *params)
{
    VGPLOG("call");
    return (virgl_renderer_gl_context)0;
}

static void pipe_virgl_destroy_context(
    void *opaque, virgl_renderer_gl_context ctx)
{
    VGPLOG("call");
}

static int pipe_virgl_make_context_current(
    void *opaque, int scanout_idx, virgl_renderer_gl_context ctx)
{
    VGPLOG("call");
    return 0;
}

static struct virgl_renderer_callbacks s_pipe_3d_cbs = {
    .version             = 1,
    .write_fence         = pipe_virgl_write_fence,
    .create_gl_context   = pipe_virgl_create_context,
    .destroy_gl_context  = pipe_virgl_destroy_context,
    .make_current        = pipe_virgl_make_context_current,
};

struct virgl_renderer_virtio_interface*
get_goldfish_pipe_virgl_renderer_virtio_interface(void) {
    return &s_virtio_interface;
}

struct virgl_renderer_callbacks*
get_goldfish_pipe_virgl_renderer_callbacks(void) {
    return &s_pipe_3d_cbs;
}

void virtio_goldfish_pipe_reset(void *pipe, void *host_pipe) {
    sRenderer->resetPipe((GoldfishHwPipe*)pipe, (GoldfishHostPipe*)host_pipe);
}

} // extern "C"

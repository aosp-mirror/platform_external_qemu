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
#include "android/base/AlignedBuf.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/memory/LazyInstance.h"
#include "android/emulation/AddressSpaceService.h"
#include "android/emulation/address_space_device.h"
#include "android/emulation/android_pipe_common.h"
#include "android/emulation/HostmemIdMapping.h"
#include "android/opengles.h"

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

#define VGP_FATAL(fmt,...) do { \
    fprintf(stderr, "virto-goldfish-pipe fatal error: %s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); \
    abort(); \
} while(0);

#ifdef VIRTIO_GOLDFISH_EXPORT_API

#ifdef _WIN32
#define VG_EXPORT __declspec(dllexport)
#else
#define VG_EXPORT __attribute__((visibility("default")))
#endif

#else

#define VG_EXPORT

#endif // !VIRTIO_GOLDFISH_EXPORT_API

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
// side. The sync_iov helper function converts convert the list of pointers
// to one contiguous buffer on the host (or vice versa), at the cost of a copy.
// (TODO: see if we can use host coherent memory to do away with the copy).
//
// We can see this abstraction in use via the implementation of
// transferWriteIov and transferReadIov below, which sync the iovec to/from a
// linear buffer if necessary, and then perform a corresponding pip operation
// based on the box parameter's x and width values.

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;
using android::emulation::HostmemIdMapping;

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
    uint32_t addressSpaceHandle;
    bool hasAddressSpaceHandle;
};

struct PipeResEntry {
    virgl_renderer_resource_create_args args;
    iovec* iov;
    uint32_t numIovs;
    void* linear;
    size_t linearSize;
    GoldfishHostPipe* hostPipe;
    VirglCtxId ctxId;
    uint64_t hva;
    uint64_t hvaSize;
    uint64_t hvaId;
    uint32_t hvSlot;
};

static inline uint32_t align_up(uint32_t n, uint32_t a) {
    return ((n + a - 1) / a) * a;
}

#define VIRGL_FORMAT_NV12 166
#define VIRGL_FORMAT_YV12 163

const uint32_t kGlBgra = 0x80e1;
const uint32_t kGlRgba = 0x1908;
const uint32_t kGlRgb565 = 0x8d62;
const uint32_t kGlR8 = 0x8229;
const uint32_t kGlRg8 = 0x822b;
const uint32_t kGlLuminance = 0x1909;
const uint32_t kGlLuminanceAlpha = 0x190a;
const uint32_t kGlUnsignedByte = 0x1401;
const uint32_t kGlUnsignedShort565 = 0x8363;

constexpr uint32_t kFwkFormatGlCompat = 0;
constexpr uint32_t kFwkFormatYV12 = 1;
constexpr uint32_t kFwkFormatYUV420888 = 2;
constexpr uint32_t kFwkFormatNV12 = 3;

static inline bool virgl_format_is_yuv(uint32_t format) {
    switch (format) {
        case VIRGL_FORMAT_B8G8R8X8_UNORM:
        case VIRGL_FORMAT_B8G8R8A8_UNORM:
        case VIRGL_FORMAT_R8G8B8X8_UNORM:
        case VIRGL_FORMAT_R8G8B8A8_UNORM:
        case VIRGL_FORMAT_B5G6R5_UNORM:
        case VIRGL_FORMAT_R8_UNORM:
        case VIRGL_FORMAT_R8G8_UNORM:
            return false;
        case VIRGL_FORMAT_NV12:
        case VIRGL_FORMAT_YV12:
            return true;
        default:
            VGP_FATAL("Unknown virgl format: 0x%x", format);
    }
}

static inline uint32_t virgl_format_to_bpp(uint32_t format) {
    uint32_t bpp = 4U;

    switch (format) {
        case VIRGL_FORMAT_R8_UNORM:
            bpp = 1U;
            break;
        case VIRGL_FORMAT_R8G8_UNORM:
        case VIRGL_FORMAT_B5G6R5_UNORM:
            bpp = 2U;
            break;
        case VIRGL_FORMAT_B8G8R8A8_UNORM:
        case VIRGL_FORMAT_B8G8R8X8_UNORM:
        case VIRGL_FORMAT_R8G8B8A8_UNORM:
        case VIRGL_FORMAT_R8G8B8X8_UNORM:
        default:
            bpp = 4U;
            break;
    }

    return bpp;
}

static inline uint32_t virgl_format_to_gl(uint32_t virgl_format) {
    switch (virgl_format) {
        case VIRGL_FORMAT_B8G8R8X8_UNORM:
        case VIRGL_FORMAT_B8G8R8A8_UNORM:
            return kGlBgra;
        case VIRGL_FORMAT_R8G8B8X8_UNORM:
        case VIRGL_FORMAT_R8G8B8A8_UNORM:
            return kGlRgba;
        case VIRGL_FORMAT_B5G6R5_UNORM:
            return kGlRgb565;
        case VIRGL_FORMAT_R8_UNORM:
            return kGlR8;
        case VIRGL_FORMAT_R8G8_UNORM:
            return kGlRg8;
        case VIRGL_FORMAT_NV12:
        case VIRGL_FORMAT_YV12:
            // emulated as RGBA8888
            return kGlRgba;
        default:
            return kGlRgba;
    }
}

static inline uint32_t virgl_format_to_fwk_format(uint32_t virgl_format) {
    switch (virgl_format) {
        case VIRGL_FORMAT_NV12:
            return kFwkFormatNV12;
        case VIRGL_FORMAT_YV12:
            return kFwkFormatYV12;
        case VIRGL_FORMAT_R8_UNORM:
        case VIRGL_FORMAT_R8G8_UNORM:
        case VIRGL_FORMAT_B8G8R8X8_UNORM:
        case VIRGL_FORMAT_B8G8R8A8_UNORM:
        case VIRGL_FORMAT_R8G8B8X8_UNORM:
        case VIRGL_FORMAT_R8G8B8A8_UNORM:
        case VIRGL_FORMAT_B5G6R5_UNORM:
        default: // kFwkFormatGlCompat: No extra conversions needed
            return kFwkFormatGlCompat;
    }
}

static inline uint32_t gl_format_to_natural_type(uint32_t format) {
    switch (format) {
        case kGlBgra:
        case kGlRgba:
        case kGlLuminance:
        case kGlLuminanceAlpha:
            return kGlUnsignedByte;
        case kGlRgb565:
            return kGlUnsignedShort565;
        default:
            return kGlUnsignedByte;
    }
}

static inline size_t virgl_format_to_linear_base(
    uint32_t format,
    uint32_t totalWidth, uint32_t totalHeight,
    uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (virgl_format_is_yuv(format)) {
        return 0;
    } else {
        uint32_t bpp = 4;
        switch (format) {
            case VIRGL_FORMAT_B8G8R8X8_UNORM:
            case VIRGL_FORMAT_B8G8R8A8_UNORM:
            case VIRGL_FORMAT_R8G8B8X8_UNORM:
            case VIRGL_FORMAT_R8G8B8A8_UNORM:
                bpp = 4;
                break;
            case VIRGL_FORMAT_B5G6R5_UNORM:
            case VIRGL_FORMAT_R8G8_UNORM:
                bpp = 2;
                break;
            case VIRGL_FORMAT_R8_UNORM:
                bpp = 1;
                break;
            default:
                VGP_FATAL("Unknown format: 0x%x", format);
        }

        uint32_t stride = totalWidth * bpp;
        return y * stride + x * bpp;
    }
    return 0;
}

static inline size_t virgl_format_to_total_xfer_len(
    uint32_t format,
    uint32_t totalWidth, uint32_t totalHeight,
    uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (virgl_format_is_yuv(format)) {
        uint32_t align = (format == VIRGL_FORMAT_YV12) ?  1 : 16;
        uint32_t yStride = (totalWidth + (align - 1)) & ~(align-1);
        uint32_t uvStride = (yStride / 2 + (align - 1)) & ~(align-1);
        uint32_t uvHeight = totalHeight / 2;
        uint32_t dataSize = yStride * totalHeight + 2 * (uvHeight * uvStride);
        return dataSize;
    } else {
        uint32_t bpp = 4;
        switch (format) {
            case VIRGL_FORMAT_B8G8R8X8_UNORM:
            case VIRGL_FORMAT_B8G8R8A8_UNORM:
            case VIRGL_FORMAT_R8G8B8X8_UNORM:
            case VIRGL_FORMAT_R8G8B8A8_UNORM:
                bpp = 4;
                break;
            case VIRGL_FORMAT_B5G6R5_UNORM:
            case VIRGL_FORMAT_R8G8_UNORM:
                bpp = 2;
                break;
            case VIRGL_FORMAT_R8_UNORM:
                bpp = 1;
                break;
            default:
                VGP_FATAL("Unknown format: 0x%x", format);
        }

        uint32_t stride = totalWidth * bpp;
        return (h - 1U) * stride + w * bpp;
    }
    return 0;
}


enum IovSyncDir {
    IOV_TO_LINEAR = 0,
    LINEAR_TO_IOV = 1,
};

static int sync_iov(PipeResEntry* res, uint64_t offset, const virgl_box* box, IovSyncDir dir) {
    VGPLOG("offset: 0x%llx box: %u %u %u %u size %u x %u iovs %u linearSize %zu",
            (unsigned long long)offset,
            box->x, box->y, box->w, box->h,
            res->args.width, res->args.height,
            res->numIovs,
            res->linearSize);

    if (box->x > res->args.width || box->y > res->args.height) {
        VGP_FATAL("Box out of range of resource");
    }
    if (box->w == 0U || box->h == 0U) {
        VGP_FATAL("Empty transfer");
    }
    if (box->x + box->w > res->args.width) {
        VGP_FATAL("Box overflows resource width");
    }

    size_t linearBase = virgl_format_to_linear_base(
        res->args.format,
        res->args.width,
        res->args.height,
        box->x, box->y, box->w, box->h);
    size_t start = linearBase;
    // height - 1 in order to treat the (w * bpp) row specially
    // (i.e., the last row does not occupy the full stride)
    size_t length = virgl_format_to_total_xfer_len(
        res->args.format,
        res->args.width,
        res->args.height,
        box->x, box->y, box->w, box->h);
    size_t end = start + length;

    if (end > res->linearSize) {
        VGP_FATAL("start + length overflows! linearSize %zu, start %zu length %zu (wanted %zu)",
                  res->linearSize, start, length, start + length);
    }

    uint32_t iovIndex = 0;
    size_t iovOffset = 0;
    bool first = true;
    size_t written = 0;
    char* linear = static_cast<char*>(res->linear);

    while (written < length) {

        if (iovIndex >= res->numIovs) {
            VGP_FATAL("write request overflowed numIovs");
        }

        const char* iovBase_const = static_cast<const char*>(res->iov[iovIndex].iov_base);
        char* iovBase = static_cast<char*>(res->iov[iovIndex].iov_base);
        size_t iovLen = res->iov[iovIndex].iov_len;
        size_t iovOffsetEnd = iovOffset + iovLen;

        auto lower_intersect = std::max(iovOffset, start);
        auto upper_intersect = std::min(iovOffsetEnd, end);
        if (lower_intersect < upper_intersect) {
            size_t toWrite = upper_intersect - lower_intersect;
            switch (dir) {
                case IOV_TO_LINEAR:
                    memcpy(linear + lower_intersect,
                           iovBase_const + lower_intersect - iovOffset,
                           toWrite);
                    break;
                case LINEAR_TO_IOV:
                    memcpy(iovBase + lower_intersect - iovOffset,
                           linear + lower_intersect,
                           toWrite);
                    break;
                default:
                    VGP_FATAL("Invalid sync dir: %d", dir);
            }
            written += toWrite;
        }
        ++iovIndex;
        iovOffset += iovLen;
    }

    return 0;
}

static uint64_t convert32to64(uint32_t lo, uint32_t hi) {
    return ((uint64_t)lo) | (((uint64_t)hi) << 32);
}

// Commands for address space device
// kVirtioGpuAddressSpaceContextCreateWithSubdevice | subdeviceType
const uint32_t kVirtioGpuAddressSpaceContextCreateWithSubdevice = 0x1001;

// kVirtioGpuAddressSpacePing | offset_lo | offset_hi | metadata_lo | metadata_hi | version | wait_fd | wait_flags | direction
// no output
const uint32_t kVirtioGpuAddressSpacePing = 0x1002;

// kVirtioGpuAddressSpacePingWithResponse | resp_resid | offset_lo | offset_hi | metadata_lo | metadata_hi | version | wait_fd | wait_flags | direction
// out: same as input then | out: error
const uint32_t kVirtioGpuAddressSpacePingWithResponse = 0x1003;

// Commands for native sync fd
const uint32_t kVirtioGpuNativeSyncCreateExportFd = 0x9000;
const uint32_t kVirtioGpuNativeSyncCreateImportFd = 0x9001;

class PipeVirglRenderer {
public:
    PipeVirglRenderer() = default;

    int init(void* cookie, int flags, const struct virgl_renderer_callbacks* callbacks) {
        VGPLOG("cookie: %p", cookie);
        mCookie = cookie;
        mVirglRendererCallbacks = *callbacks;
        mVirtioGpuOps = android_getVirtioGpuOps();
        if (!mVirtioGpuOps) {
            VGP_FATAL("Could not get virtio gpu ops!");
        }
        mReadPixelsFunc = android_getReadPixelsFunc();
        if (!mReadPixelsFunc) {
            VGP_FATAL("Could not get read pixels func!");
        }
        mAddressSpaceDeviceControlOps = get_address_space_device_control_ops();
        if (!mAddressSpaceDeviceControlOps) {
            VGP_FATAL("Could not get address space device control ops!");
        }
        VGPLOG("done");
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
            0, // AS handle
            false, // does not have an AS handle
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

        if (it->second.hasAddressSpaceHandle) {
            mAddressSpaceDeviceControlOps->destroy_handle(
                it->second.addressSpaceHandle);
        }

        auto ops = ensureAndGetServiceOps();
        auto hostPipe = it->second.hostPipe;

        if (!hostPipe) {
            fprintf(stderr, "%s: 0 is not a valid hostpipe\n", __func__);
            return -1;
        }

        ops->guest_close(hostPipe, GOLDFISH_PIPE_CLOSE_GRACEFUL);

        mContexts.erase(it);
        return 0;
    }

    void setContextAddressSpaceHandleLocked(VirglCtxId ctxId, uint32_t handle) {
        auto ctxIt = mContexts.find(ctxId);
        if (ctxIt == mContexts.end()) {
            fprintf(stderr, "%s: fatal: ctx id %u not found\n", __func__,
                    ctxId);
            abort();
        }

        auto& ctxEntry = ctxIt->second;
        ctxEntry.addressSpaceHandle = handle;
        ctxEntry.hasAddressSpaceHandle = true;
    }

    uint32_t getAddressSpaceHandleLocked(VirglCtxId ctxId) {
        auto ctxIt = mContexts.find(ctxId);
        if (ctxIt == mContexts.end()) {
            fprintf(stderr, "%s: fatal: ctx id %u not found\n", __func__,
                    ctxId);
            abort();
        }

        auto& ctxEntry = ctxIt->second;

        if (!ctxEntry.hasAddressSpaceHandle) {
            fprintf(stderr, "%s: fatal: ctx id %u doesn't have address space handle\n", __func__,
                    ctxId);
            abort();
        }

        return ctxEntry.addressSpaceHandle;
    }

    void writeWordsToFirstIovPageLocked(uint32_t* dwords, size_t dwordCount, uint32_t resId) {

        auto resEntryIt = mResources.find(resId);
        if (resEntryIt == mResources.end()) {
            fprintf(stderr, "%s: fatal: resid %u not found\n", __func__, resId);
            abort();
        }

        auto& resEntry = resEntryIt->second;

        if (!resEntry.iov) {
            fprintf(stderr, "%s: fatal:resid %u had empty iov\n", __func__, resId);
            abort();
        }

        uint32_t* iovWords = (uint32_t*)(resEntry.iov[0].iov_base);
        memcpy(iovWords, dwords, sizeof(uint32_t) * dwordCount);
    }

    void addressSpaceProcessCmd(VirglCtxId ctxId, uint32_t* dwords, int dwordCount) {
        uint32_t opcode = dwords[0];

        switch (opcode) {
            case kVirtioGpuAddressSpaceContextCreateWithSubdevice: {
                uint32_t subdevice_type = dwords[1];

                uint32_t handle = mAddressSpaceDeviceControlOps->gen_handle();

                struct android::emulation::AddressSpaceDevicePingInfo pingInfo = {
                    .metadata = (uint64_t)subdevice_type,
                };

                mAddressSpaceDeviceControlOps->ping_at_hva(handle, &pingInfo);

                AutoLock lock(mLock);
                setContextAddressSpaceHandleLocked(ctxId, handle);
                break;
            }
            case kVirtioGpuAddressSpacePing: {
                uint32_t phys_addr_lo = dwords[1];
                uint32_t phys_addr_hi = dwords[2];

                uint32_t size_lo = dwords[3];
                uint32_t size_hi = dwords[4];

                uint32_t metadata_lo = dwords[5];
                uint32_t metadata_hi = dwords[6];

                uint32_t wait_phys_addr_lo = dwords[7];
                uint32_t wait_phys_addr_hi = dwords[8];

                uint32_t wait_flags = dwords[9];
                uint32_t direction = dwords[10];

                struct android::emulation::AddressSpaceDevicePingInfo pingInfo = {
                    .phys_addr = convert32to64(phys_addr_lo, phys_addr_hi),
                    .size = convert32to64(size_lo, size_hi),
                    .metadata = convert32to64(metadata_lo, metadata_hi),
                    .wait_phys_addr = convert32to64(wait_phys_addr_lo, wait_phys_addr_hi),
                    .wait_flags = wait_flags,
                    .direction = direction,
                };

                AutoLock lock(mLock);
                mAddressSpaceDeviceControlOps->ping_at_hva(
                    getAddressSpaceHandleLocked(ctxId),
                    &pingInfo);
                break;
            }
            case kVirtioGpuAddressSpacePingWithResponse: {
                uint32_t resp_resid = dwords[1];
                uint32_t phys_addr_lo = dwords[2];
                uint32_t phys_addr_hi = dwords[3];

                uint32_t size_lo = dwords[4];
                uint32_t size_hi = dwords[5];

                uint32_t metadata_lo = dwords[6];
                uint32_t metadata_hi = dwords[7];

                uint32_t wait_phys_addr_lo = dwords[8];
                uint32_t wait_phys_addr_hi = dwords[9];

                uint32_t wait_flags = dwords[10];
                uint32_t direction = dwords[11];

                struct android::emulation::AddressSpaceDevicePingInfo pingInfo = {
                    .phys_addr = convert32to64(phys_addr_lo, phys_addr_hi),
                    .size = convert32to64(size_lo, size_hi),
                    .metadata = convert32to64(metadata_lo, metadata_hi),
                    .wait_phys_addr = convert32to64(wait_phys_addr_lo, wait_phys_addr_hi),
                    .wait_flags = wait_flags,
                    .direction = direction,
                };

                AutoLock lock(mLock);
                mAddressSpaceDeviceControlOps->ping_at_hva(
                    getAddressSpaceHandleLocked(ctxId),
                    &pingInfo);

                phys_addr_lo = (uint32_t)pingInfo.phys_addr;
                phys_addr_hi = (uint32_t)(pingInfo.phys_addr >> 32);
                size_lo = (uint32_t)(pingInfo.size >> 0);
                size_hi = (uint32_t)(pingInfo.size >> 32);
                metadata_lo = (uint32_t)(pingInfo.metadata >> 0);
                metadata_hi = (uint32_t)(pingInfo.metadata >> 32);
                wait_phys_addr_lo = (uint32_t)(pingInfo.wait_phys_addr >> 0);
                wait_phys_addr_hi = (uint32_t)(pingInfo.wait_phys_addr >> 32);
                wait_flags = (uint32_t)(pingInfo.wait_flags >> 0);
                direction = (uint32_t)(pingInfo.direction >> 0);

                uint32_t response[] = {
                    phys_addr_lo, phys_addr_hi,
                    size_lo, size_hi,
                    metadata_lo, metadata_hi,
                    wait_phys_addr_lo, wait_phys_addr_hi,
                    wait_flags, direction,
                };

                writeWordsToFirstIovPageLocked(
                    response,
                    sizeof(response) / sizeof(uint32_t),
                    resp_resid);
                break;
            }
            default:
                break;
        }
    }

    int submitCmd(VirglCtxId ctxId, void* buffer, int dwordCount) {
        VGPLOG("ctxid: %u buffer: %p dwords: %d", ctxId, buffer, dwordCount);

        if (!buffer) {
            fprintf(stderr, "%s: error: buffer null\n", __func__);
            return -1;
        }

        // Parse command from buffer
        uint32_t* dwords = (uint32_t*)buffer;

        if (dwordCount < 1) {
            fprintf(stderr, "%s: error: not enough dwords (got %d)\n", __func__, dwordCount);
            return -1;
        }

        uint32_t opcode = dwords[0];

        switch (opcode) {
            case kVirtioGpuAddressSpaceContextCreateWithSubdevice:
            case kVirtioGpuAddressSpacePing:
            case kVirtioGpuAddressSpacePingWithResponse:
                addressSpaceProcessCmd(ctxId, dwords, dwordCount);
                break;
            case kVirtioGpuNativeSyncCreateExportFd:
            case kVirtioGpuNativeSyncCreateImportFd: {
                uint32_t sync_handle_lo = dwords[1];
                uint32_t sync_handle_hi = dwords[2];
                uint64_t sync_handle =
                    (((uint64_t)sync_handle_hi) << 32) |
                    ((uint64_t)sync_handle_lo);
                mVirtioGpuOps->wait_for_gpu(sync_handle);
                break;
            }
            default:
                return -1;
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
            VGPLOG("write fence: %u (done with callback)", fence);
        }
        mFenceDeque.clear();
        VGPLOG("end");
    }

    enum pipe_texture_target {
        PIPE_BUFFER,
        PIPE_TEXTURE_1D,
        PIPE_TEXTURE_2D,
        PIPE_TEXTURE_3D,
        PIPE_TEXTURE_CUBE,
        PIPE_TEXTURE_RECT,
        PIPE_TEXTURE_1D_ARRAY,
        PIPE_TEXTURE_2D_ARRAY,
        PIPE_TEXTURE_CUBE_ARRAY,
        PIPE_MAX_TEXTURE_TYPES,
    };

    /**
     *  * Resource binding flags -- state tracker must specify in advance all
     *   * the ways a resource might be used.
     *    */
#define PIPE_BIND_DEPTH_STENCIL        (1 << 0) /* create_surface */
#define PIPE_BIND_RENDER_TARGET        (1 << 1) /* create_surface */
#define PIPE_BIND_BLENDABLE            (1 << 2) /* create_surface */
#define PIPE_BIND_SAMPLER_VIEW         (1 << 3) /* create_sampler_view */
#define PIPE_BIND_VERTEX_BUFFER        (1 << 4) /* set_vertex_buffers */
#define PIPE_BIND_INDEX_BUFFER         (1 << 5) /* draw_elements */
#define PIPE_BIND_CONSTANT_BUFFER      (1 << 6) /* set_constant_buffer */
#define PIPE_BIND_DISPLAY_TARGET       (1 << 7) /* flush_front_buffer */
    /* gap */
#define PIPE_BIND_STREAM_OUTPUT        (1 << 10) /* set_stream_output_buffers */
#define PIPE_BIND_CURSOR               (1 << 11) /* mouse cursor */
#define PIPE_BIND_CUSTOM               (1 << 12) /* state-tracker/winsys usages */
#define PIPE_BIND_GLOBAL               (1 << 13) /* set_global_binding */
#define PIPE_BIND_SHADER_BUFFER        (1 << 14) /* set_shader_buffers */
#define PIPE_BIND_SHADER_IMAGE         (1 << 15) /* set_shader_images */
#define PIPE_BIND_COMPUTE_RESOURCE     (1 << 16) /* set_compute_resources */
#define PIPE_BIND_COMMAND_ARGS_BUFFER  (1 << 17) /* pipe_draw_info.indirect */
#define PIPE_BIND_QUERY_BUFFER         (1 << 18) /* get_query_result_resource */


    void handleCreateResourceGraphicsUsage(
            struct virgl_renderer_resource_create_args *args,
            struct iovec *iov, uint32_t num_iovs) {

        if (args->target == PIPE_BUFFER) {
            // Nothing to handle; this is generic pipe usage.
            return;
        }

        // corresponds to allocation of gralloc buffer in minigbm
        VGPLOG("w h %u %u resid %u -> rcCreateColorBufferWithHandle",
               args->width, args->height, args->handle);
        uint32_t glformat = virgl_format_to_gl(args->format);
        uint32_t fwkformat = virgl_format_to_fwk_format(args->format);
        mVirtioGpuOps->create_color_buffer_with_handle(
            args->width, args->height, glformat, fwkformat, args->handle);
    }

    int createResource(
            struct virgl_renderer_resource_create_args *args,
            struct iovec *iov, uint32_t num_iovs) {

        VGPLOG("handle: %u. num iovs: %u", args->handle, num_iovs);

        handleCreateResourceGraphicsUsage(args, iov, num_iovs);

        PipeResEntry e;
        e.args = *args;
        e.linear = 0;
        e.hostPipe = 0;
        e.hva = 0;
        e.hvaSize = 0;
        e.hvaId = 0;
        e.hvSlot = 0;
        allocResource(e, iov, num_iovs);

        AutoLock lock(mLock);
        mResources[args->handle] = e;
        return 0;
    }

    void handleUnrefResourceGraphicsUsage(PipeResEntry* res, uint32_t resId) {
        if (res->args.target == PIPE_BUFFER) return;
        mVirtioGpuOps->close_color_buffer(resId);
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

        handleUnrefResourceGraphicsUsage(&entry, toUnrefId);

        if (entry.linear) {
            free(entry.linear);
            entry.linear = nullptr;
        }

        if (entry.iov) {
            free(entry.iov);
            entry.iov = nullptr;
            entry.numIovs = 0;
        }

        if (entry.hvaId) {
            // gfxstream manages when to actually remove the hostmem id and storage
            //
            // fprintf(stderr, "%s: unref a hostmem resource. hostmem id: 0x%llx\n", __func__,
            //         (unsigned long long)(entry.hvaId));
            // HostmemIdMapping::get()->remove(entry.hvaId);
            // auto ownedIt = mOwnedHostmemIdBuffers.find(entry.hvaId);
            // if (ownedIt != mOwnedHostmemIdBuffers.end()) {
            //      // android::aligned_buf_free(ownedIt->second);
            // }
        }

        entry.hva = 0;
        entry.hvaSize = 0;
        entry.hvaId = 0;
        entry.hvSlot = 0;
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

    bool handleTransferReadGraphicsUsage(
        PipeResEntry* res, uint64_t offset, virgl_box* box) {
        // PIPE_BUFFER: Generic pipe usage
        if (res->args.target == PIPE_BUFFER) return true;

        // Others: Gralloc transfer read operation
        auto glformat = virgl_format_to_gl(res->args.format);
        auto gltype = gl_format_to_natural_type(glformat);

        // We always xfer the whole thing again from GL
        // since it's fiddly to calc / copy-out subregions
        if (virgl_format_is_yuv(res->args.format)) {
            mVirtioGpuOps->read_color_buffer_yuv(
                res->args.handle,
                0, 0,
                res->args.width, res->args.height,
                res->linear, res->linearSize);
        } else {
            mVirtioGpuOps->read_color_buffer(
                res->args.handle,
                0, 0,
                res->args.width, res->args.height,
                glformat,
                gltype,
                res->linear);
        }

        return false;
    }

    bool handleTransferWriteGraphicsUsage(
        PipeResEntry* res, uint64_t offset, virgl_box* box) {
        // PIPE_BUFFER: Generic pipe usage
        if (res->args.target == PIPE_BUFFER) return true;

        // Others: Gralloc transfer read operation
        auto glformat = virgl_format_to_gl(res->args.format);
        auto gltype = gl_format_to_natural_type(glformat);

        // We always xfer the whole thing again to GL
        // since it's fiddly to calc / copy-out subregions
        mVirtioGpuOps->update_color_buffer(
            res->args.handle,
            0, 0,
            res->args.width, res->args.height,
            glformat,
            gltype,
            res->linear);

        return false;
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

        if (handleTransferReadGraphicsUsage(
            &entry, offset, box)) {
            // Do the pipe service op here, if there is an associated hostpipe.
            auto hostPipe = entry.hostPipe;
            if (!hostPipe) return -1;

            auto ops = ensureAndGetServiceOps();

            size_t readBytes = 0;
            size_t wantedBytes = readBytes + (size_t)box->w;

            while (readBytes < wantedBytes) {
                GoldfishPipeBuffer buf = {
                    ((char*)entry.linear) + box->x + readBytes,
                    wantedBytes - readBytes,
                };
                auto status = ops->guest_recv(hostPipe, &buf, 1);

                if (status > 0) {
                    readBytes += status;
                } else if (status != kPipeTryAgain) {
                    return EIO;
                }
            }
        }

        VGPLOG("Linear first word: %d", *(int*)(entry.linear));

        auto syncRes = sync_iov(&entry, offset, box, LINEAR_TO_IOV);
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
        auto syncRes = sync_iov(&entry, offset, box, IOV_TO_LINEAR);

        if (handleTransferWriteGraphicsUsage(&entry, offset, box)) {
            // Do the pipe service op here, if there is an associated hostpipe.
            auto hostPipe = entry.hostPipe;
            if (!hostPipe) {
                VGPLOG("No hostPipe");
                return syncRes;
            }

            VGPLOG("resid: %d offset: 0x%llx hostpipe: %p", resId,
                   (unsigned long long)offset, hostPipe);

            auto ops = ensureAndGetServiceOps();

            size_t writtenBytes = 0;
            size_t wantedBytes = (size_t)box->w;

            while (writtenBytes < wantedBytes) {
                GoldfishPipeBuffer buf = {
                    ((char*)entry.linear) + box->x + writtenBytes,
                    wantedBytes - writtenBytes,
                };
                auto status = ops->guest_send(hostPipe, &buf, 1);

                if (status > 0) {
                    writtenBytes += status;
                } else if (status != kPipeTryAgain) {
                    return EIO;
                }
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

    void flushResourceAndReadback(
        uint32_t res_handle, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
        void* pixels, uint32_t max_bytes) {
        (void)x;
        (void)y;
        (void)width;
        (void)height;
        //TODO: displayId > 0 ?
        uint32_t displayId = 0;
        mVirtioGpuOps->post_color_buffer(res_handle);
        if (pixels) {
            mReadPixelsFunc(pixels, max_bytes, displayId);
        }
    }

    void createResourceV2(uint32_t res_handle, uint64_t hvaId) {
        PipeResEntry e;
        struct virgl_renderer_resource_create_args args = {
            res_handle,
            PIPE_BUFFER,
            VIRGL_FORMAT_R8_UNORM,
            PIPE_BIND_COMMAND_ARGS_BUFFER,
            0, 1, 1,
            0, 0, 0, 0
        };
        e.args = args;
        e.hostPipe = 0;

        auto entry = HostmemIdMapping::get()->get(hvaId);

        e.hva = entry.hva;
        e.hvaSize = entry.size;
        e.args.width = entry.size;
        e.hvaId = hvaId;
        e.hvSlot = 0;
        e.iov = nullptr;
        e.numIovs = 0;
        e.linear = 0;
        e.linearSize = 0;

        AutoLock lock(mLock);
        mResources[res_handle] = e;
    }

    uint64_t getResourceHva(uint32_t res_handle) {
        AutoLock lock(mLock);
        auto it = mResources.find(res_handle);
        if (it == mResources.end()) return 0;
        const auto& entry = it->second;
        return entry.hva;
    }

    uint64_t getResourceHvaSize(uint32_t res_handle) {
        AutoLock lock(mLock);
        auto it = mResources.find(res_handle);
        if (it == mResources.end()) return 0;
        const auto& entry = it->second;
        return entry.hvaSize;
    }

    int resourceMap(uint32_t res_handle, void** hvaOut, uint64_t* sizeOut) {
        AutoLock lock(mLock);
        auto it = mResources.find(res_handle);
        if (it == mResources.end()) {
            if (hvaOut) *hvaOut = nullptr;
            if (sizeOut) *sizeOut = 0;
            return -1;
        }

        const auto& entry = it->second;

        static const uint64_t kPageSizeforBlob = 4096;
        static const uint64_t kPageMaskForBlob = ~(0xfff);

        uint64_t alignedHva =
            entry.hva & kPageMaskForBlob;

        uint64_t alignedSize =
            kPageSizeforBlob *
            ((entry.hvaSize + kPageSizeforBlob - 1) / kPageSizeforBlob);

        if (hvaOut) *hvaOut = (void*)(uintptr_t)alignedHva;
        if (sizeOut) *sizeOut = alignedSize;
        return 0;
    }

    int resourceUnmap(uint32_t res_handle) {
        AutoLock lock(mLock);
        auto it = mResources.find(res_handle);
        if (it == mResources.end()) {
            return -1;
        }

        // TODO(lfy): Good place to run any registered cleanup callbacks.
        // No-op for now.
        return 0;
    }

    void setResourceHvSlot(uint32_t res_handle, uint32_t slot) {
        AutoLock lock(mLock);
        auto it = mResources.find(res_handle);
        if (it == mResources.end()) return;
        auto& entry = it->second;
        entry.hvSlot = slot;
    }

    uint32_t getResourceHvSlot(uint32_t res_handle) {
        AutoLock lock(mLock);
        auto it = mResources.find(res_handle);
        if (it == mResources.end()) return 0;
        const auto& entry = it->second;
        return entry.hvSlot;
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

    inline const GoldfishPipeServiceOps* ensureAndGetServiceOps() {
        if (mServiceOps) return mServiceOps;
        mServiceOps = goldfish_pipe_get_service_ops();
        return mServiceOps;
    }

    Lock mLock;

    void* mCookie = nullptr;
    int mFlags = 0;
    virgl_renderer_callbacks mVirglRendererCallbacks;
    AndroidVirtioGpuOps* mVirtioGpuOps = nullptr;
    ReadPixelsFunc mReadPixelsFunc = nullptr;
    struct address_space_device_control_ops* mAddressSpaceDeviceControlOps =
        nullptr;

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

VG_EXPORT int pipe_virgl_renderer_init(
    void *cookie, int flags, struct virgl_renderer_callbacks *cb) {
    sRenderer->init(cookie, flags, cb);
    return 0;
}

VG_EXPORT void pipe_virgl_renderer_poll(void) {
    sRenderer->poll();
}

VG_EXPORT void* pipe_virgl_renderer_get_cursor_data(
    uint32_t resource_id, uint32_t *width, uint32_t *height) {
    return 0;
}

VG_EXPORT int pipe_virgl_renderer_resource_create(
    struct virgl_renderer_resource_create_args *args,
    struct iovec *iov, uint32_t num_iovs) {

    return sRenderer->createResource(args, iov, num_iovs);
}

VG_EXPORT void pipe_virgl_renderer_resource_unref(uint32_t res_handle) {
    sRenderer->unrefResource(res_handle);
}

VG_EXPORT int pipe_virgl_renderer_context_create(
    uint32_t handle, uint32_t nlen, const char *name) {
    return sRenderer->createContext(handle, nlen, name);
}

VG_EXPORT void pipe_virgl_renderer_context_destroy(uint32_t handle) {
    sRenderer->destroyContext(handle);
}

VG_EXPORT int pipe_virgl_renderer_submit_cmd(void *buffer,
                                          int ctx_id,
                                          int dwordCount) {
    return sRenderer->submitCmd(ctx_id, buffer, dwordCount);
}

VG_EXPORT int pipe_virgl_renderer_transfer_read_iov(
    uint32_t handle, uint32_t ctx_id,
    uint32_t level, uint32_t stride,
    uint32_t layer_stride,
    struct virgl_box *box,
    uint64_t offset, struct iovec *iov,
    int iovec_cnt) {
    return sRenderer->transferReadIov(handle, offset, box);
}

VG_EXPORT int pipe_virgl_renderer_transfer_write_iov(
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
VG_EXPORT void pipe_virgl_renderer_get_cap_set(uint32_t, uint32_t*, uint32_t*) { }
VG_EXPORT void pipe_virgl_renderer_fill_caps(uint32_t, uint32_t, void *caps) { }

VG_EXPORT int pipe_virgl_renderer_resource_attach_iov(
    int res_handle, struct iovec *iov,
    int num_iovs) {
    return sRenderer->attachIov(res_handle, iov, num_iovs);
}

VG_EXPORT void pipe_virgl_renderer_resource_detach_iov(
    int res_handle, struct iovec **iov, int *num_iovs) {
    return sRenderer->detachIov(res_handle, iov, num_iovs);
}

VG_EXPORT int pipe_virgl_renderer_create_fence(
    int client_fence_id, uint32_t cmd_type) {
    sRenderer->createFence(client_fence_id, cmd_type);
    return 0;
}

VG_EXPORT void pipe_virgl_renderer_force_ctx_0(void) {
    VGPLOG("call");
}

VG_EXPORT void pipe_virgl_renderer_ctx_attach_resource(
    int ctx_id, int res_handle) {
    sRenderer->attachResource(ctx_id, res_handle);
}

VG_EXPORT void pipe_virgl_renderer_ctx_detach_resource(
    int ctx_id, int res_handle) {
    sRenderer->detachResource(ctx_id, res_handle);
}

VG_EXPORT int pipe_virgl_renderer_resource_get_info(
    int res_handle,
    struct virgl_renderer_resource_info *info) {
    return sRenderer->getResourceInfo(res_handle, info);
}

VG_EXPORT int pipe_virgl_renderer_resource_create_v2(uint32_t res_handle, uint64_t hvaId) {
    sRenderer->createResourceV2(res_handle, hvaId);
    return 0;
}

VG_EXPORT int pipe_virgl_renderer_resource_map(uint32_t res_handle, void** hvaOut, uint64_t* sizeOut) {
    return sRenderer->resourceMap(res_handle, hvaOut, sizeOut);
}

VG_EXPORT int pipe_virgl_renderer_resource_unmap(uint32_t res_handle) {
    return sRenderer->resourceUnmap(res_handle);
}

VG_EXPORT void stream_renderer_flush_resource_and_readback(
    uint32_t res_handle, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
    void* pixels, uint32_t max_bytes) {
    sRenderer->flushResourceAndReadback(res_handle, x, y, width, height, pixels, max_bytes);
}

VG_EXPORT void stream_renderer_resource_create_v2(
    uint32_t res_handle, uint64_t hvaId) {
    sRenderer->createResourceV2(res_handle, hvaId);
}

VG_EXPORT uint64_t stream_renderer_resource_get_hva(uint32_t res_handle) {
    return sRenderer->getResourceHva(res_handle);
}

VG_EXPORT uint64_t stream_renderer_resource_get_hva_size(uint32_t res_handle) {
    return sRenderer->getResourceHvaSize(res_handle);
}

VG_EXPORT void stream_renderer_resource_set_hv_slot(uint32_t res_handle, uint32_t slot) {
    sRenderer->setResourceHvSlot(res_handle, slot);
}

VG_EXPORT uint32_t stream_renderer_resource_get_hv_slot(uint32_t res_handle) {
    return sRenderer->getResourceHvSlot(res_handle);
}

VG_EXPORT int stream_renderer_resource_map(uint32_t res_handle, void** hvaOut, uint64_t* sizeOut) {
    return sRenderer->resourceMap(res_handle, hvaOut, sizeOut);
}

VG_EXPORT int stream_renderer_resource_unmap(uint32_t res_handle) {
    return sRenderer->resourceUnmap(res_handle);
}


#define VIRGLRENDERER_API_PIPE_STRUCT_DEF(api) pipe_##api,

static struct virgl_renderer_virtio_interface s_virtio_interface = {
    LIST_VIRGLRENDERER_API(VIRGLRENDERER_API_PIPE_STRUCT_DEF)
};

struct virgl_renderer_virtio_interface*
get_goldfish_pipe_virgl_renderer_virtio_interface(void) {
    return &s_virtio_interface;
}

void virtio_goldfish_pipe_reset(void *pipe, void *host_pipe) {
    sRenderer->resetPipe((GoldfishHwPipe*)pipe, (GoldfishHostPipe*)host_pipe);
}

} // extern "C"

/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "RenderThread.h"

#include "ChannelStream.h"
#include "RingStream.h"
#include "FrameBuffer.h"
#include "ReadBuffer.h"
#include "RenderControl.h"
#include "RendererImpl.h"
#include "RenderChannelImpl.h"
#include "RenderThreadInfo.h"

#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "OpenGLESDispatch/GLESv1Dispatch.h"
#include "../../../shared/OpenglCodecCommon/ChecksumCalculatorThreadInfo.h"

#include "android/base/system/System.h"
#include "aemu/base/Tracing.h"
#include "aemu/base/files/StreamSerializing.h"
#include "aemu/base/synchronization/Lock.h"
#include "android/utils/path.h"
#include "android/utils/file_io.h"

#define EMUGL_DEBUG_LEVEL 0
#include "host-common/crash_reporter.h"
#include "host-common/debug.h"
#include "emugl/common/logging.h"

#include <assert.h>

using android::base::AutoLock;

namespace emugl {

struct RenderThread::SnapshotObjects {
    RenderThreadInfo* threadInfo;
    ChecksumCalculator* checksumCalc;
    ChannelStream* channelStream;
    RingStream* ringStream;
    ReadBuffer* readBuffer;
};

static bool getBenchmarkEnabledFromEnv() {
    auto threadEnabled = android::base::System::getEnvironmentVariable("ANDROID_EMUGL_RENDERTHREAD_STATS");
    if (threadEnabled == "1") return true;
    return false;
}

static uint64_t currTimeUs(bool enable) {
    if (enable) {
        return android::base::System::get()->getHighResTimeUs();
    } else {
        return 0;
    }
}

// Start with a smaller buffer to not waste memory on a low-used render threads.
static constexpr int kStreamBufferSize = 128 * 1024;

// Requires this many threads on the system available to run unlimited.
static constexpr int kMinThreadsToRunUnlimited = 5;

// A thread run limiter that limits render threads to run one slice at a time.
static android::base::Lock sThreadRunLimiter;

RenderThread::RenderThread(RenderChannelImpl* channel,
                           android::base::Stream* loadStream)
    : emugl::Thread(android::base::ThreadFlags::MaskSignals, 2 * 1024 * 1024),
      mChannel(channel),
      mRunInLimitedMode(android::base::System::get()->getCpuCoreCount() < kMinThreadsToRunUnlimited) {
    if (loadStream) {
        const bool success = loadStream->getByte();
        if (success) {
            mStream.emplace(0);
            android::base::loadStream(loadStream, &*mStream);
            mState = SnapshotState::StartLoading;
        } else {
            mFinished.store(true, std::memory_order_relaxed);
        }
    }
}

RenderThread::RenderThread(
        struct asg_context context,
        android::base::Stream* loadStream,
        android::emulation::asg::ConsumerCallbacks callbacks,
        uint32_t virtioGpuContextId, uint32_t virtioGpuCapsetId,
        std::optional<std::string> nameOpt)
    : android::base::Thread(android::base::ThreadFlags::MaskSignals, 2 * 1024 * 1024,
                            std::move(nameOpt)),
      mRingStream(
          new RingStream(context, callbacks, kStreamBufferSize)),
      mVirtioGpuContextId(virtioGpuContextId), mVirtioGpuCapsetId(virtioGpuCapsetId) {
    if (loadStream) {
        const bool success = loadStream->getByte();
        if (success) {
            mStream.emplace(0);
            android::base::loadStream(loadStream, &*mStream);
            mState = SnapshotState::StartLoading;
        } else {
            mFinished.store(true, std::memory_order_relaxed);
        }
    }
}


RenderThread::~RenderThread() = default;

void RenderThread::pausePreSnapshot() {
    AutoLock lock(mLock);
    assert(mState == SnapshotState::Empty);
    mStream.emplace();
    mState = SnapshotState::StartSaving;
    if (mRingStream) {
        mRingStream->pausePreSnapshot();
        // mCondVar.broadcastAndUnlock(&lock);
    }
    if (mChannel) {
        mChannel->pausePreSnapshot();
        mCondVar.broadcastAndUnlock(&lock);
    }
}

void RenderThread::resume() {
    AutoLock lock(mLock);
    // This function can be called for a thread from pre-snapshot loading
    // state; it doesn't need to do anything.
    if (mState == SnapshotState::Empty) {
        return;
    }
    if (mRingStream) mRingStream->resume();
    waitForSnapshotCompletion(&lock);
    mStream.clear();
    mState = SnapshotState::Empty;
    if (mChannel) mChannel->resume();
    if (mRingStream) mRingStream->resume();
    mCondVar.broadcastAndUnlock(&lock);
}

void RenderThread::save(android::base::Stream* stream) {
    bool success;
    {
        AutoLock lock(mLock);
        assert(mState == SnapshotState::StartSaving ||
               mState == SnapshotState::InProgress ||
               mState == SnapshotState::Finished);
        waitForSnapshotCompletion(&lock);
        success = mState == SnapshotState::Finished;
    }

    if (success) {
        assert(mStream);
        stream->putByte(1);
        android::base::saveStream(stream, *mStream);
    } else {
        stream->putByte(0);
    }
}

void RenderThread::waitForSnapshotCompletion(AutoLock* lock) {
    while (mState != SnapshotState::Finished &&
           !mFinished.load(std::memory_order_relaxed)) {
        mCondVar.wait(lock);
    }
}

template <class OpImpl>
void RenderThread::snapshotOperation(AutoLock* lock, OpImpl&& implFunc) {
    assert(isPausedForSnapshotLocked());
    mState = SnapshotState::InProgress;
    mCondVar.broadcastAndUnlock(lock);

    implFunc();

    lock->lock();

    mState = SnapshotState::Finished;
    mCondVar.broadcast();

    // Only return after we're allowed to proceed.
    while (isPausedForSnapshotLocked()) {
        mCondVar.wait(lock);
    }
}

void RenderThread::loadImpl(AutoLock* lock, const SnapshotObjects& objects) {
    snapshotOperation(lock, [this, &objects] {
        objects.readBuffer->onLoad(&*mStream);
        if (objects.channelStream) objects.channelStream->load(&*mStream);
        if (objects.ringStream) objects.ringStream->load(&*mStream);
        objects.checksumCalc->load(&*mStream);
        objects.threadInfo->onLoad(&*mStream);
    });
}

void RenderThread::saveImpl(AutoLock* lock, const SnapshotObjects& objects) {
    snapshotOperation(lock, [this, &objects] {
        objects.readBuffer->onSave(&*mStream);
        if (objects.channelStream) objects.channelStream->save(&*mStream);
        if (objects.ringStream) objects.ringStream->save(&*mStream);
        objects.checksumCalc->save(&*mStream);
        objects.threadInfo->onSave(&*mStream);
    });
}

bool RenderThread::isPausedForSnapshotLocked() const {
    return mState != SnapshotState::Empty;
}

bool RenderThread::doSnapshotOperation(const SnapshotObjects& objects,
                                       SnapshotState state) {
    AutoLock lock(mLock);
    if (mState == state) {
        switch (state) {
            case SnapshotState::StartLoading:
                loadImpl(&lock, objects);
                return true;
            case SnapshotState::StartSaving:
                saveImpl(&lock, objects);
                return true;
            default:
                return false;
        }
    }
    return false;
}

void RenderThread::setFinished() {
    // Make sure it never happens that we wait forever for the thread to
    // save to snapshot while it was not even going to.
    AutoLock lock(mLock);
    mFinished.store(true, std::memory_order_relaxed);
    setOnTeardownLocked([] {});
    if (mState != SnapshotState::Empty) {
        mCondVar.broadcastAndUnlock(&lock);
    }
}

void RenderThread::notifyTeardown() {
    AutoLock lock(mLock);
    if (!isFinished()) {
        mOnTeardown();
        setOnTeardownLocked([] {});
    }
}

intptr_t RenderThread::main() {
    if (mFinished.load(std::memory_order_relaxed)) {
        ERR("Error: fail loading a RenderThread @%p", this);
        return 0;
    }

    RenderThreadInfo tInfo;
    ChecksumCalculatorThreadInfo tChecksumInfo;
    ChecksumCalculator& checksumCalc = tChecksumInfo.get();
    bool needRestoreFromSnapshot = false;

    //
    // initialize decoders
    //
    tInfo.m_glDec.initGL(gles1_dispatch_get_proc_func, nullptr);
    tInfo.m_gl2Dec.initGL(gles2_dispatch_get_proc_func, nullptr);
    initRenderControlContext(&tInfo.m_rcDec);
    {
        AutoLock lock(mLock);
        // This callback is only used when a RenderThread::main() is running
        // and will be cleared when `tInfo` goes out of scope, so it's safe to
        // use `&tInfo` here.
        setOnTeardownLocked([this, &tInfo] {
            D("RenderThread @%p gets teardown request. puid=%ul", this,
              tInfo.m_puid);

            // If this callback is called, it indicates that other components
            // requested a teardown of this RenderThread; the most probable
            // cause is the address space stream is destroyed. In such cases,
            // it won't make sense for decoders to wait on the global command
            // sequences, because the command it waits might never arrive. So
            // all the render threads associated with this process should skip
            // waiting for seqnos, until a new stream (and new process context)
            // is recreated for this process later.
            if (tInfo.m_puid) {
                const ProcessResources* processResources =
                        FrameBuffer::getFB()->getProcessResources(tInfo.m_puid);

                // This is the only place skipSeqnoWait is set and we only need
                // atomicity for it, so we can just use relaxed memory order.
                processResources->getSkipWaitingForSequenceNumber()->store(
                        true, std::memory_order_relaxed);
            }

            tInfo.m_vkDec.onThreadTeardown();
        });
    }

    if (!mChannel && !mRingStream) {
        GL_LOG("Exited a loader RenderThread @%p", this);
        AutoLock lock(mLock);
        mFinished.store(true, std::memory_order_relaxed);
        setOnTeardownLocked([] {});
        return 0;
    }

    ChannelStream stream(mChannel, RenderChannel::Buffer::kSmallSize);
    IOStream* ioStream =
        mChannel ? (IOStream*)&stream : (IOStream*)mRingStream.get();

    ReadBuffer readBuf(kStreamBufferSize);
    if (mRingStream) {
        readBuf.setNeededFreeTailSize(0);
    }

    const SnapshotObjects snapshotObjects = {
        &tInfo, &checksumCalc, &stream, mRingStream.get(), &readBuf,
    };

    // Framebuffer initialization is asynchronous, so we need to make sure
    // it's completely initialized before running any GL commands.
    FrameBuffer::waitUntilInitialized();

    // This is the only place where we try loading from snapshot.
    // But the context bind / restoration will be delayed after receiving
    // the first GL command.
    if (doSnapshotOperation(snapshotObjects, SnapshotState::StartLoading)) {
        GL_LOG("Loaded RenderThread @%p from snapshot", this);
        needRestoreFromSnapshot = true;
    } else {
        // Not loading from a snapshot: continue regular startup, read
        // the |flags|.
        uint32_t flags = 0;
        while (ioStream->read(&flags, sizeof(flags)) != sizeof(flags)) {
            // Stream read may fail because of a pending snapshot.
            if (!doSnapshotOperation(snapshotObjects, SnapshotState::StartSaving)) {
                setFinished();
                GL_LOG("Exited a RenderThread @%p early", this);
                return 0;
            }
        }

        // |flags| used to mean something, now they're not used.
        (void)flags;
    }

    int stats_totalBytes = 0;
    uint64_t stats_progressTimeUs = 0;
    auto stats_t0 = android::base::System::get()->getHighResTimeUs() / 1000;
    bool benchmarkEnabled = getBenchmarkEnabledFromEnv();

    //
    // open dump file if RENDER_DUMP_DIR is defined
    //
    const char* dump_dir = getenv("RENDERER_DUMP_DIR");
    FILE* dumpFP = nullptr;
    if (dump_dir) {
        size_t bsize = strlen(dump_dir) + 32;
        char* fname = new char[bsize];
        snprintf(fname, bsize, "%s" PATH_SEP "stream_%p", dump_dir, this);
        dumpFP = android_fopen(fname, "wb");
        if (!dumpFP) {
            fprintf(stderr, "Warning: stream dump failed to open file %s\n",
                    fname);
        }
        delete[] fname;
    }

    const ProcessResources* processResources = nullptr;

    while (1) {
        // Let's make sure we read enough data for at least some processing.
        int packetSize;
        if (readBuf.validData() >= 8) {
            // We know that packet size is the second int32_t from the start.
            packetSize = *(const int32_t*)(readBuf.buf() + 4);
            if (!packetSize) {
                // Emulator will get live-stuck here if packet size is read to be zero;
                // crash right away so we can see these events.
                emugl::emugl_crash_reporter(
                    "Guest should never send a size-0 GL packet\n");
            }
        } else {
            // Read enough data to at least be able to get the packet size next
            // time.
            packetSize = 8;
        }

        int stat = 0;
        if (packetSize > (int)readBuf.validData()) {
            stat = readBuf.getData(ioStream, packetSize);
            if (stat <= 0) {
                if (doSnapshotOperation(snapshotObjects, SnapshotState::StartSaving)) {
                    continue;
                } else {
                    D("Warning: render thread could not read data from stream");
                    break;
                }
            } else if (needRestoreFromSnapshot) {
                // If we're using RingStream that might load before FrameBuffer
                // restores the contexts from the handles, so check again here.

                tInfo.postLoadRefreshCurrentContextSurfacePtrs();
                // We just loaded from a snapshot, need to initialize / bind
                // the contexts.
                needRestoreFromSnapshot = false;
                HandleType ctx = tInfo.currContext ? tInfo.currContext->getHndl()
                        : 0;
                HandleType draw = tInfo.currDrawSurf ? tInfo.currDrawSurf->getHndl()
                        : 0;
                HandleType read = tInfo.currReadSurf ? tInfo.currReadSurf->getHndl()
                        : 0;
                FrameBuffer::getFB()->bindContext(ctx, draw, read);
            }
        }

        DD("render thread read %d bytes, op %d, packet size %d",
           (int)readBuf.validData(), *(int32_t*)readBuf.buf(),
           *(int32_t*)(readBuf.buf() + 4));

        //
        // log received bandwidth statistics
        //
        if (benchmarkEnabled) {
            stats_totalBytes += readBuf.validData();
            auto dt = android::base::System::get()->getHighResTimeUs() / 1000 - stats_t0;
            if (dt > 1000) {
                float dts = (float)dt / 1000.0f;
                printf("Used Bandwidth %5.3f MB/s, time in progress %f ms total %f ms\n", ((float)stats_totalBytes / dts) / (1024.0f*1024.0f),
                        stats_progressTimeUs / 1000.0f,
                        (float)dt);
                readBuf.printStats();
                stats_t0 = android::base::System::get()->getHighResTimeUs() / 1000;
                stats_progressTimeUs = 0;
                stats_totalBytes = 0;
            }
        }

        //
        // dump stream to file if needed
        //
        if (dumpFP) {
            int skip = readBuf.validData() - stat;
            fwrite(readBuf.buf() + skip, 1, readBuf.validData() - skip, dumpFP);
            fflush(dumpFP);
        }

        auto progressStart = currTimeUs(benchmarkEnabled);
        bool progress;

        do {

            if (!processResources && tInfo.m_puid) {
                processResources = FrameBuffer::getFB()->getProcessResources(tInfo.m_puid);
            }

            if (mRunInLimitedMode) {
                sThreadRunLimiter.lock();
            }

            progress = false;

            size_t last;

            //
            // try to process some of the command buffer using the
            // Vulkan decoder
            //
            {
                last = tInfo.m_vkDec.decode(readBuf.buf(), readBuf.validData(),
                                            ioStream, processResources);
                if (last > 0) {
                    readBuf.consume(last);
                    progress = true;
                }
            }

            // try to process some of the command buffer using the GLESv1
            // decoder
            //
            // DRIVER WORKAROUND:
            // On Linux with NVIDIA GPU's at least, we need to avoid performing
            // GLES ops while someone else holds the FrameBuffer write lock.
            //
            // To be more specific, on Linux with NVIDIA Quadro K2200 v361.xx,
            // we get a segfault in the NVIDIA driver when glTexSubImage2D
            // is called at the same time as glXMake(Context)Current.
            //
            // To fix, this driver workaround avoids calling
            // any sort of GLES call when we are creating/destroying EGL
            // contexts.
            {
                FrameBuffer::getFB()->lockContextStructureRead();
            }

            {
                last = tInfo.m_glDec.decode(
                        readBuf.buf(), readBuf.validData(), ioStream, &checksumCalc);
                if (last > 0) {
                    progress = true;
                    readBuf.consume(last);
                }
            }

            //
            // try to process some of the command buffer using the GLESv2
            // decoder
            //
            {
                last = tInfo.m_gl2Dec.decode(readBuf.buf(), readBuf.validData(),
                                             ioStream, &checksumCalc);

                if (last > 0) {
                    progress = true;
                    readBuf.consume(last);
                }
            }

            FrameBuffer::getFB()->unlockContextStructureRead();
            //
            // try to process some of the command buffer using the
            // renderControl decoder
            //
            {
                last = tInfo.m_rcDec.decode(readBuf.buf(), readBuf.validData(),
                                            ioStream, &checksumCalc);
                if (last > 0) {
                    readBuf.consume(last);
                    progress = true;
                }
            }

            if (mRunInLimitedMode) {
                sThreadRunLimiter.unlock();
            }

        } while (progress);
    }

    if (dumpFP) {
        fclose(dumpFP);
    }

    // Don't check for snapshots here: if we're already exiting then snapshot
    // should not contain this thread information at all.
    if (!FrameBuffer::getFB()->isShuttingDown()) {
        tInfo.release();
        // Release references to the current thread's context/surfaces if any
        FrameBuffer::getFB()->bindContext(0, 0, 0);
        if (tInfo.currContext || tInfo.currDrawSurf || tInfo.currReadSurf) {
            ERR("ERROR: RenderThread exiting with current context/surfaces");
        }

        FrameBuffer::getFB()->drainWindowSurface();
        FrameBuffer::getFB()->drainRenderContext();
    }

    setFinished();

    GL_LOG("Exited a RenderThread @%p", this);
    return 0;
}

}  // namespace emugl

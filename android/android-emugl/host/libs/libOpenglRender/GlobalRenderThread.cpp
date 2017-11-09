#include "GlobalRenderThread.h"

#include "FrameBuffer.h"

#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/threads/WorkerThread.h"

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::WorkerThread;
using android::base::WorkerProcessingResult;

namespace emugl {

class GlobalRenderWorker {
public:
    struct Cmd {
        RenderThreadInfo* tInfo;
        GLESv1Decoder* gl1;
        GLESv2Decoder* gl2;
        renderControl_decoder_context_t* rc;
        unsigned char* buf;
        size_t datalen;
        ChannelStream* stream;
        ChecksumCalculatorThreadInfo* checksumCalcThreadInfo;
    };

    GlobalRenderWorker() { }
    void doDecode(Cmd& cmd) {
        RenderThreadInfo::setCurrent(cmd.tInfo);

        if (m_lastThreadInfo &&
            cmd.tInfo != m_lastThreadInfo) {

            uint32_t contextHandle = 0;
            uint32_t drawSurfaceHandle = 0;
            uint32_t readSurfaceHandle = 0;
            if (cmd.tInfo->currContext) {
                contextHandle = cmd.tInfo->currContext->getHndl();
            }
            if (cmd.tInfo->currDrawSurf) {
                drawSurfaceHandle = cmd.tInfo->currDrawSurf->getHndl();
            }
            if (cmd.tInfo->currReadSurf) {
                readSurfaceHandle = cmd.tInfo->currReadSurf->getHndl();
            }
            FrameBuffer::getFB()->bindContext(contextHandle, drawSurfaceHandle, readSurfaceHandle);

        }

        m_lastThreadInfo = cmd.tInfo;
        ChecksumCalculatorThreadInfo::setCurrent(cmd.checksumCalcThreadInfo);
        ChecksumCalculator* calc = &cmd.checksumCalcThreadInfo->get();

        mProgress = 0;

        size_t gl1Progress =
            cmd.gl1->decode(
                    cmd.buf, cmd.datalen, cmd.stream, calc);
        if (gl1Progress > 0) {
            cmd.buf += (uintptr_t) gl1Progress;
            mProgress += gl1Progress;
            cmd.datalen -= gl1Progress;
        }

        size_t gl2Progress =
            cmd.gl2->decode(
                    cmd.buf, cmd.datalen, cmd.stream, calc);
        if (gl2Progress > 0) {
            cmd.buf += (uintptr_t) gl2Progress;
            mProgress += gl2Progress;
            cmd.datalen -= gl2Progress;
        }

        size_t rcProgress =
            cmd.rc->decode(
                    cmd.buf, cmd.datalen, cmd.stream, calc);
        if (rcProgress > 0) {
            cmd.buf += (uintptr_t) rcProgress;
            mProgress += rcProgress;
            cmd.datalen -= rcProgress;
        }
    }

    size_t getLastProgress() {
        return mProgress;
    }
    android::base::Lock& lock() {
        return mLock;
    }
private:
    RenderThreadInfo* m_lastThreadInfo = nullptr;
    android::base::Lock mLock;
    size_t mProgress = 0;
};

static LazyInstance<GlobalRenderWorker> sWorker = LAZY_INSTANCE_INIT;
static WorkerThread<GlobalRenderWorker::Cmd>* sWorkerThread = nullptr;

GlobalRenderThread::GlobalRenderThread() {
    sWorkerThread = new WorkerThread<GlobalRenderWorker::Cmd>([](GlobalRenderWorker::Cmd&& cmd) {
        sWorker->doDecode(cmd);
        return WorkerProcessingResult::Continue;
    });
    sWorkerThread->start();
}

size_t GlobalRenderThread::decode(RenderThreadInfo* tInfo,
                                  GLESv1Decoder* gl1,
                                  GLESv2Decoder* gl2,
                                  renderControl_decoder_context_t* rc,
                                  unsigned char* buf, size_t datalen,
                                  ChannelStream* stream,
                                  ChecksumCalculatorThreadInfo* checksumCalcThreadInfo) {
    AutoLock lock(sWorker->lock());
    GlobalRenderWorker::Cmd cmd;
    cmd.tInfo = tInfo;
    cmd.gl1 = gl1;
    cmd.gl2 = gl2;
    cmd.rc = rc;
    cmd.buf = buf;
    cmd.datalen = datalen;
    cmd.stream = stream;
    cmd.checksumCalcThreadInfo = checksumCalcThreadInfo;
    sWorkerThread->enqueue(GlobalRenderWorker::Cmd(cmd));
    sWorkerThread->waitQueuedItems();
    return sWorker->getLastProgress();
}

} // namespace emugl

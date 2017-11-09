#include "GlobalRenderThread.h"

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
        GLESv1Decoder* gl1;
        GLESv2Decoder* gl2;
        renderControl_decoder_context_t* rc;
        unsigned char* buf;
        size_t datalen;
        ChannelStream* stream;
        ChecksumCalculator* checksumCalc;
    };

    GlobalRenderWorker() { }
    void doDecode(Cmd& cmd) {
        mProgress = 0;

        fprintf(stderr, "%s: gl1 %p 2 %p rc %p\n", __func__, cmd.gl1, cmd.gl2, cmd.rc);

        size_t gl1Progress =
            cmd.gl1->decode(
                    cmd.buf, cmd.datalen, cmd.stream, cmd.checksumCalc);
        if (gl1Progress > 0) {
            fprintf(stderr, "%s: gl1 progress\n", __func__);
            cmd.buf += (uintptr_t) gl1Progress;
            mProgress += gl1Progress;
            cmd.datalen -= gl1Progress;
        }
        else {
            fprintf(stderr, "%s: no gl1 progress %d\n", __func__, gl1Progress);
        }

        size_t gl2Progress =
            cmd.gl2->decode(
                    cmd.buf, cmd.datalen, cmd.stream, cmd.checksumCalc);
        if (gl2Progress > 0) {
            fprintf(stderr, "%s: gl2Progress\n", __func__);
            cmd.buf += (uintptr_t) gl2Progress;
            mProgress += gl2Progress;
            cmd.datalen -= gl2Progress;
        }
        else {
            fprintf(stderr, "%s: no gl2 progress %d\n", __func__, gl2Progress);
        }

        size_t rcProgress =
            cmd.rc->decode(
                    cmd.buf, cmd.datalen, cmd.stream, cmd.checksumCalc);
        if (rcProgress > 0) {
            fprintf(stderr, "%s: rcProgress\n", __func__);
            cmd.buf += (uintptr_t) rcProgress;
            mProgress += rcProgress;
            cmd.datalen -= rcProgress;
        }
        else {
            fprintf(stderr, "%s: norc progress %d\n", __func__, rcProgress);
        }
    }

    size_t getLastProgress() {
        fprintf(stderr, "%s: %zu\n", __func__, mProgress);
        return mProgress;
    }
    android::base::Lock& lock() {
        return mLock;
    }
private:
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

size_t GlobalRenderThread::decode(GLESv1Decoder* gl1,
                                  GLESv2Decoder* gl2,
                                  renderControl_decoder_context_t* rc,
                                  unsigned char* buf, size_t datalen,
                                  ChannelStream* stream,
                                  ChecksumCalculator* checksumCalc) {
    AutoLock lock(sWorker->lock());
    GlobalRenderWorker::Cmd cmd;
    cmd.gl1 = gl1;
    cmd.gl2 = gl2;
    cmd.rc = rc;
    cmd.buf = buf;
    cmd.datalen = datalen;
    cmd.stream = stream;
    cmd.checksumCalc = checksumCalc;
    sWorkerThread->enqueue(GlobalRenderWorker::Cmd(cmd));
    sWorkerThread->waitQueuedItems();
    return sWorker->getLastProgress();
}

} // namespace emugl

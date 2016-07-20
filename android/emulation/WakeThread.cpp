
#include "android/emulation/WakeThread.h"

WakeThread::WakeThread(android::AndroidPipe* pipe) {
    mPipe = pipe;
    exiting = false;
    this->start(); 
}

bool WakeThread::wakePipe() {
    if (mPipe && !exiting) {
        mPipe->signalWake(0);
    }
    return !exiting;
}

intptr_t WakeThread::main() {
    mLooper = android::base::ThreadLooper::get();
    mTask = new android::base::RecurrentTask(mLooper, std::bind(&WakeThread::wakePipe, this), 1000);
    fprintf(stderr, "WakeThread::%s: start\n", __FUNCTION__);
    mTask->start();
    mLooper->run();
    fprintf(stderr, "WakeThread::%s: stop\n", __FUNCTION__);
    return 0;
}


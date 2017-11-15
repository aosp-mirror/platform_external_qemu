#include "DriverThread.h"

#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/threads/ThreadStore.h"
#include "android/base/threads/WorkerThread.h"

#include <GLES3/gl3.h>

#include <stdio.h>

using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;
using android::base::ThreadStore;
using android::base::WorkerThread;
using android::base::WorkerProcessingResult;

class DriverThreadWorker {
public:
    enum CallType {
        NormalApi = 0,
        Context = 1,
    };

    enum ReturnType {
        Void = 0,
        Glbool = 1,
        Glint = 2,
        Glenum = 3,
        Glsync = 4,
        Voidptr = 5,
        Constglubyteptr = 6,
        Unsignedlongint = 7,
    };

    struct Call {
        CallType callType;
        ReturnType retType;
        std::function<void()> funcVoid;
        std::function<GLboolean()> funcBool;
        std::function<GLint()> funcInt;
        std::function<GLenum()> funcEnum;
        std::function<GLsync()> funcSync;
        std::function<void*()> funcVoidptr;
        std::function<const GLubyte*()> funcConstglubyteptr;
        std::function<unsigned long int()> funcUnsignedlongint;
        DriverThread::ContextInfo contextInfo;
        bool changeContext;
    };

    DriverThreadWorker() :
        mWorkerThread([this](Call&& call) { return runOne(call); }) {
        mWorkerThread.start();
    }

    void registerContextQueryCallback(const DriverThread::ContextQueryCallback& c) {
        mContextQueryCallback = c;
    }

    void registerContextChangeCallback(const DriverThread::ContextChangeCallback& c) {
        mContextChangeCallback = c;
    }

    GLboolean getRetValBoolean() { return mBooleanReturn; }
    GLint getRetValInt() { return mIntReturn; }
    GLenum getRetValEnum() { return mEnumReturn; }
    GLsync getRetValSync() { return mSyncReturn; }
    void* getRetValVoidptr() { return mVoidptrReturn; }
    const GLubyte* getRetValConstglubyteptr() { return mConstglubyteptr; }

    WorkerProcessingResult runOne(const Call& call) {
        if (call.callType == CallType::NormalApi) {
            switch (call.retType) {
                case ReturnType::Void:
                    call.funcVoid();
                    break;
                case ReturnType::Glbool:
                    mBooleanReturn = call.funcBool();
                    break;
                case ReturnType::Glint:
                    mIntReturn = call.funcInt();
                    break;
                case ReturnType::Glenum:
                    mEnumReturn = call.funcEnum();
                    break;
                case ReturnType::Glsync:
                    mSyncReturn = call.funcSync();
                    break;
                case ReturnType::Voidptr:
                    mVoidptrReturn = call.funcVoidptr();
                    break;
                case ReturnType::Constglubyteptr:
                    mConstglubyteptr = call.funcConstglubyteptr();
                    break;
                case ReturnType::Unsignedlongint:
                    mUnsignedlongintReturn = call.funcUnsignedlongint();
                    break;
                default:
                    break;
            }
        } else if (call.callType == CallType::Context) {
            mBooleanReturn = true;
            if (call.contextInfo.context != mCurrContextInfo.context ||
                call.contextInfo.read != mCurrContextInfo.read ||
                call.contextInfo.draw != mCurrContextInfo.draw) {
                if (mContextChangeCallback && call.changeContext) {
                    //mBooleanReturn = mContextChangeCallback({0, 0, 0});
                    mBooleanReturn = mContextChangeCallback(call.contextInfo);
                }
            }
            mCurrContextInfo = call.contextInfo;
        }
        return WorkerProcessingResult::Continue;
    }

    void callCommon_locked(bool changeContext) {
        Call c;
        c.callType = CallType::Context;
        c.changeContext = changeContext;
        if (mContextQueryCallback) {
            c.contextInfo = mContextQueryCallback();
        }
        enqueue(c);
    }

    bool makeCurrent(const DriverThread::ContextInfo& contextInfo) {
        AutoLock lock(mLock);
        Call c;
        c.callType = CallType::Context;
        c.changeContext = true;
        c.contextInfo = contextInfo;
        enqueue(c); flush(); mWorkerThread.waitQueuedItems();
        return mBooleanReturn;
    }

    bool makeCurrentToCallingThread(const DriverThread::ContextInfo& contextInfo) {
        mPerThreadContextInfo.set(new DriverThread::ContextInfo(contextInfo));
        return true;
    }

    DriverThread::ContextInfo getCallingThreadContextInfo() {
        if (mPerThreadContextInfo.get()) {
            return *mPerThreadContextInfo.get();
        } else {
            return { 0, 0, 0 };
        }
    }

    void callVoid(std::function<void()> f, bool changeContext) {
        AutoLock lock(mLock);
        flush();
        callCommon_locked(changeContext);
        Call c;
        c.callType = CallType::NormalApi;
        c.retType = ReturnType::Void;
        c.funcVoid = f;
        enqueue(c); flush(); mWorkerThread.waitQueuedItems();
        return void();
    }

    void callVoidAsync(std::function<void()> f, bool changeContext) {
        AutoLock lock(mLock);

        callCommon_locked(changeContext);
        Call c;
        c.callType = CallType::NormalApi;
        c.retType = ReturnType::Void;
        c.funcVoid = f;
        // enqueue(c);
        enqueue(c); flush(); mWorkerThread.waitQueuedItems();
        return void();
    }

    GLboolean callBoolean(std::function<GLboolean()> f, bool changeContext) {
        AutoLock lock(mLock);
        flush();
        callCommon_locked(changeContext);
        Call c;
        c.callType = CallType::NormalApi;
        c.retType = ReturnType::Glbool;
        c.funcBool = f;
        enqueue(c); flush(); mWorkerThread.waitQueuedItems();

        return mBooleanReturn;
    }

    GLint callInt(std::function<GLint()> f, bool changeContext) {
        AutoLock lock(mLock);
        flush();
        callCommon_locked(changeContext);
        Call c;
        c.callType = CallType::NormalApi;
        c.retType = ReturnType::Glint;
        c.funcInt = f;
        enqueue(c); flush(); mWorkerThread.waitQueuedItems();

        return mIntReturn;
    }

    GLenum callEnum(std::function<GLenum()> f, bool changeContext) {
        AutoLock lock(mLock);
        flush();
        callCommon_locked(changeContext);
        Call c;
        c.callType = CallType::NormalApi;
        c.retType = ReturnType::Glenum;
        c.funcEnum = f;
        enqueue(c); flush(); mWorkerThread.waitQueuedItems();

        return mEnumReturn;
    }

    GLsync callSync(std::function<GLsync()> f, bool changeContext) {
        AutoLock lock(mLock);
        flush();
        callCommon_locked(changeContext);
        Call c;
        c.callType = CallType::NormalApi;
        c.retType = ReturnType::Glsync;
        c.funcSync = f;
        enqueue(c); flush(); mWorkerThread.waitQueuedItems();

        return mSyncReturn;
    }

    void* callVoidptr(std::function<void*()> f, bool changeContext) {
        AutoLock lock(mLock);
        flush();
        callCommon_locked(changeContext);
        Call c;
        c.callType = CallType::NormalApi;
        c.retType = ReturnType::Voidptr;
        c.funcVoidptr = f;
        enqueue(c); flush(); mWorkerThread.waitQueuedItems();

        return mVoidptrReturn;
    }

    unsigned long int callUnsignedLongInt(std::function<unsigned long int()> f, bool changeContext) {
        AutoLock lock(mLock);
        flush();
        callCommon_locked(changeContext);
        Call c;
        c.callType = CallType::NormalApi;
        c.retType = ReturnType::Unsignedlongint;
        c.funcUnsignedlongint = f;
        enqueue(c); flush(); mWorkerThread.waitQueuedItems();

        return mUnsignedlongintReturn;
    }

    const GLubyte* callConstglubyteptr(std::function<const GLubyte*()> f, bool changeContext) {
        AutoLock lock(mLock);
        flush();
        callCommon_locked(changeContext);
        Call c;
        c.callType = CallType::NormalApi;
        c.retType = ReturnType::Constglubyteptr;
        c.funcConstglubyteptr = f;
        enqueue(c); flush(); mWorkerThread.waitQueuedItems();

        return mConstglubyteptr;
    }

    Lock& lock() { return mLock; }

    void flush() {
        mWorkerThread.enqueueMultiple(mCmdBuf);
        mCmdBuf.clear();
    }

    void enqueue(const Call& call) {
        if (mCmdBuf.size() > 200) {
            flush();
        }
        mCmdBuf.emplace_back(call);
    }

private:
    WorkerThread<Call> mWorkerThread;
    GLboolean mBooleanReturn;
    GLint mIntReturn;
    GLenum mEnumReturn;
    GLsync mSyncReturn;
    void* mVoidptrReturn;
    const GLubyte* mConstglubyteptr;
    unsigned long int mUnsignedlongintReturn;

    DriverThread::ContextQueryCallback mContextQueryCallback;
    DriverThread::ContextChangeCallback mContextChangeCallback;
    DriverThread::ContextInfo mCurrContextInfo;

    std::vector<Call> mCmdBuf = {};

    ThreadStore<DriverThread::ContextInfo> mPerThreadContextInfo;

    Lock mLock;
};

static DriverThreadWorker* sWorker = nullptr;

void DriverThread::initWorker() {
    if (!sWorker) {
        sWorker = new DriverThreadWorker;
    }
}

DriverThreadWorker* DriverThread::getWorker() {
    return sWorker;
}

void DriverThread::setWorker(DriverThreadWorker* w) {
    sWorker = w;
}

DriverThread::DriverThread() { }

void DriverThread::registerContextQueryCallback(const DriverThread::ContextQueryCallback& c) {
    if (c) sWorker->registerContextQueryCallback(c);
}

void DriverThread::registerContextChangeCallback(const DriverThread::ContextChangeCallback& c) {
    if (c) sWorker->registerContextChangeCallback(c);
}

bool DriverThread::useDriverThread() { return true; }

template <>
void DriverThread::callOnDriverThread<void>(std::function<void()> f, bool changeContext) {
    sWorker->callVoid(f, changeContext);
}

template <>
void DriverThread::callOnDriverThreadAsync<void>(std::function<void()> f, bool changeContext) {
    sWorker->callVoidAsync(f, changeContext);
}

template <>
GLboolean DriverThread::callOnDriverThread<GLboolean>(std::function<GLboolean()> f, bool changeContext) {
    return sWorker->callBoolean(f, changeContext);
}

template <>
bool DriverThread::callOnDriverThread<bool>(std::function<bool()> f, bool changeContext) {
    return sWorker->callBoolean(f, changeContext);
}

template <>
GLint DriverThread::callOnDriverThread<GLint>(std::function<GLint()> f, bool changeContext) {
    return sWorker->callInt(f, changeContext);
}

template <>
GLenum DriverThread::callOnDriverThread<GLenum>(std::function<GLenum()> f, bool changeContext) {
    return sWorker->callEnum(f, changeContext);
}

template <>
GLsync DriverThread::callOnDriverThread<GLsync>(std::function<GLsync()> f, bool changeContext) {
    return sWorker->callSync(f, changeContext);
}

template <>
void* DriverThread::callOnDriverThread<void*>(std::function<void*()> f, bool changeContext) {
    return sWorker->callVoidptr(f, changeContext);
}

template <>
const GLubyte* DriverThread::callOnDriverThread<const GLubyte*>(std::function<const  GLubyte*()> f, bool changeContext) {
    return sWorker->callConstglubyteptr(f, changeContext);
}

template <>
unsigned long int DriverThread::callOnDriverThread<unsigned long int>(std::function<unsigned long int()> f, bool changeContext) {
    return sWorker->callUnsignedLongInt(f, changeContext);
}

void DriverThread::callOnDriverThreadSync(std::function<void()> f, bool changeContext) {
    sWorker->callVoid(f, changeContext);
}

bool DriverThread::makeCurrent(const DriverThread::ContextInfo& context) {
    return sWorker->makeCurrent(context);
}

bool DriverThread::makeCurrentToCallingThread(const DriverThread::ContextInfo& context) {
    return sWorker->makeCurrentToCallingThread(context);
}

DriverThread::ContextInfo DriverThread::getCallingThreadContextInfo() {
    return sWorker->getCallingThreadContextInfo();
}

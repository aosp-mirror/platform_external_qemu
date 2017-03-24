// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/InstallPipe.h"

#include "android/install-pipe.h"

static char * s_filename = NULL;
static bool s_has_task = false;

namespace android {
namespace emulation {


InstallPipe::InstallPipe(void* hwPipe, Service* svc)
    : AndroidPipe(hwPipe, svc) {}

void InstallPipe::onGuestClose(PipeCloseReason reason) {
}

unsigned InstallPipe::onGuestPoll() const {
    unsigned flags = 0;

    // Guest can always write
    flags |= PIPE_POLL_OUT;

    // Guest may need to wait for installation request
    if (hasDataForGuest()) {
        flags |= PIPE_POLL_IN;
    }
    return flags;
}

bool InstallPipe::hasDataForGuest() {
    return mFp != NULL;
}

void openFile() {
    FILE *fp = fopen(s_filename, "web");
    if (fp) {
        fseek(fp, 0L, SEEK_END);
        int sz = ftell(fp);
        rewind(fp);
        mFp = fp;
        snprintf(mPayLoadBuf, sizeof(mPayLoadBuf), "%08x", sz);
        mPayLoadSize = strlen(mPayLoadBuf);
        mPayLoad = mPayLoadBuf;
    }
    free(s_filename);
}

int InstallPipe::copyData(char* buf, int buf_size) {
    int copied = 0;
    if (buf_size <= 0) {
        return copied;
    }
    if (s_has_task && mFp == NULL) {
        openFile();
    }
    if (mPayLoadSize > 0){
        int amount = buf_size > mPayLoadSize ? mPayLoadSize : buf_size;
        memcpy(buf, mPayLoad, amount);
        mPayLoad += amount;
        mPayLoadSize -= amount;
        if (mPayLoadSize == 0) {
            mPayLoad = NULL;
        }
        buf += amount;
        buf_size -= amount;
        copied += amount;
        if (buf_size <= 0) {
            return copied;
        }
    }
    if (mFp == NULL) {
        return copied;
    }
    copied += fread(buf, 1, buf_size, fp);
    if (copied < buf_size) {
        if (feof(mFp)) {
            fclose(mFp);
            mFp = NULL;
            s_has_task = false;
        }
    }
    return copied;
}

int InstallPipe::onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) {
    int result = 0;
    while (numBuffers > 0) {
        int copied = copyData(buffers->data, (int)(buffers->size));
        result += copied;
        if (copied < (int)(buffers->size)) {
            break;
        }
        buffers++;
        numBuffers--;
    }
    if (result == 0) {
        return PIPE_ERROR_AGAIN;
    }
    return result;
}

// guest sends back install reply
int InstallPipe::onGuestSend(const AndroidPipeBuffer* buffers,
                               int numBuffers) {
    int result = 0;
    bool replyComplete = false;
    while (numBuffers > 0) {
        char* pch = static_cast<char*>(buffers->data);
        int pos = static_cast<int>(buffers->size);
        replyComplete = (pch[pos -1] == '\0');
        mResult += std::string(static_cast<char*>(buffers->data),
                static_cast<int>(buffers->size));
        result += static_cast<int>(buffers->size);
        buffers++;
        numBuffers--;
    }
    if (replyComplete) {
        printf("Guest reply %s\n", mResult.c_str());
        s_has_task = false;
        mResult.clear();
    }
    return result;
}

void registerInstallPipeService() {
    android::AndroidPipe::Service::add(new InstallPipe::Service());
}


////////////////////////////////////////////////////////////////////////////////

InstallPipe::Service::Service() : AndroidPipe::Service("install") {}

AndroidPipe* InstallPipe::Service::create(void* hwPipe, const char* args) {
    return new InstallPipe(hwPipe, this);
}

}  // namespace emulation
}  // namespace android

void android_init_install_pipe(void) {
    android::emulation::registerInstallPipeService();
}

bool android_kick_install_pipe(const char * filename) {
    if (filename && s_has_task == false) {
        s_filename = strdup(filename);
        s_has_task = true;
        return true;
    }
    return false;
}

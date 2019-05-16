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

#include "android/emulation/LogcatPipe.h"

#include <fstream>
#include <iostream>
#include <memory>

#include "android/globals.h"
#include "android/logcat-pipe.h"
#include "android/utils/debug.h"

namespace android {
namespace emulation {

static base::Lock sLogcatStreamLock;
static std::vector<std::unique_ptr<std::ostream>> sLogcatOutputStreams;

LogcatPipe::LogcatPipe(void* hwPipe, Service* svc) : AndroidPipe(hwPipe, svc) {}

void LogcatPipe::registerStream(std::ostream* stream) {
    base::AutoLock lock(sLogcatStreamLock);
    sLogcatOutputStreams.emplace_back(std::unique_ptr<std::ostream>(stream));
}

void LogcatPipe::onGuestClose(PipeCloseReason reason) {
    delete this;
}

unsigned LogcatPipe::onGuestPoll() const {
    // Guest can always write
    return PIPE_POLL_OUT;
}

int LogcatPipe::onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) {
    // Guest is not supposed to read
    return PIPE_ERROR_IO;
}

int LogcatPipe::onGuestSend(const AndroidPipeBuffer* buffers, int numBuffers) {
    int result = 0;
    base::AutoLock lock(sLogcatStreamLock);
    while (numBuffers > 0) {
        for (auto& pstream : sLogcatOutputStreams) {
            if (pstream->good()) {
                pstream->write((char*)buffers->data,
                               static_cast<int>(buffers->size));
            }
        }

        result += static_cast<int>(buffers->size);
        buffers++;
        numBuffers--;
    }

    // Flush good streams.
    for (auto& pstream : sLogcatOutputStreams) {
        if (pstream->good()) {
            pstream->flush();
        }
    }
    return result;
}

void registerLogcatPipeService() {
    if (android_hw->hw_logcatOutput_path &&
        *android_hw->hw_logcatOutput_path != '\0') {
        std::unique_ptr<std::ofstream> outputfile(new std::ofstream(
                android_hw->hw_logcatOutput_path, std::ios_base::app));
        if (outputfile->good()) {
            sLogcatOutputStreams.push_back(std::move(outputfile));
        } else {
            dwarning("Cannot open logcat output file [%s]",
                     android_hw->hw_logcatOutput_path);
        }
    }

    android::AndroidPipe::Service::add(new LogcatPipe::Service());
}

////////////////////////////////////////////////////////////////////////////////

LogcatPipe::Service::Service() : AndroidPipe::Service("logcat") {}

AndroidPipe* LogcatPipe::Service::create(void* hwPipe, const char* args) {
    return new LogcatPipe(hwPipe, this);
}

AndroidPipe* LogcatPipe::Service::load(void* hwPipe,
                                       const char* args,
                                       android::base::Stream* stream) {
    return new LogcatPipe(hwPipe, this);
}

bool LogcatPipe::Service::canLoad() const {
    return true;
}

}  // namespace emulation
}  // namespace android

void android_init_logcat_pipe(void) {
    android::emulation::registerLogcatPipeService();
}

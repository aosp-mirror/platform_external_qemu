// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/misc/IpcPipe.h"

#include "android/base/EintrWrapper.h"
#include "android/base/files/Fd.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#endif

namespace android {
namespace base {

#ifdef _WIN32
static const unsigned int kWin32PipeBufferSize = 4096;
#endif

int ipcPipeCreate(int* readPipe, int* writePipe) {
    int fds[2];
#ifdef _WIN32
    int result = _pipe(fds, kWin32PipeBufferSize, O_BINARY | O_NOINHERIT);
#else  // _WIN32
    int result = pipe(fds);
#endif  // _WIN32
    if (result == 0) {
        fdSetCloexec(fds[0]);
        fdSetCloexec(fds[1]);
        *readPipe = fds[0];
        *writePipe = fds[1];
    }
    return result;
}

void ipcPipeClose(int pipe) {
    ::close(pipe);
}

ssize_t ipcPipeRead(int pipe, void* buffer, size_t bufferLen) {
    return HANDLE_EINTR(::read(pipe, buffer, bufferLen));
}

ssize_t ipcPipeWrite(int pipe, const void* buffer, size_t bufferLen) {
    return HANDLE_EINTR(::write(pipe, buffer, bufferLen));
}

}  // namespace base
}  // namespace android


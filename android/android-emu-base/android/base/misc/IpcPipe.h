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

#pragma once

#ifdef _MSC_VER
#include "msvc-posix.h"
#endif

#include <sys/types.h>

namespace android {
namespace base {

// Create a unidirectional pipe with the read-end connected to |readPipe| and
// the write-end connected to |writePipe|. This is equivalent to calling the
// POSIX function:
//     pipe(&fds);
// On Windows this will not use TCP loopback or sockets but instead the _pipe
// call making this an efficient form of communication on all platforms.
int ipcPipeCreate(int* readPipe, int* writePipe);

// Closes one end of a pipe. The ends of the pipe need to be closed separately.
void ipcPipeClose(int pipe);

// Try to read up to |bufferLen| bytes from |pipe| into |buffer|.
// Return the number of bytes actually read, 0 in case of disconnection,
// or -1/errno in case of error. Note that this loops around EINTR as
// a convenience. Only works on the read end of a pipe.
ssize_t ipcPipeRead(int pipe, void* buffer, size_t bufferLen);

// Try to write up to |bufferLen| bytes to |pipe| from |buffer|.
// Return the number of bytes actually written, 0 in case of disconnection,
// or -1/errno in case of error. Note that this loops around EINTR as
// a convenience. Only works on the write end of a pipe.
ssize_t ipcPipeWrite(int pipe, const void* buffer, size_t bufferLen);

}  // namespace base
}  // namespace android


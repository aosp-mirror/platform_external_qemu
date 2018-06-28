// Copyright (C) 2018 The Android Open Source Project
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
#pragma once

#include <sys/types.h>

// Fake pipe driver and device for testing goldfish pipe on host without
// running a VM.
namespace android {

class HostGoldfishPipeDevice;

// Initializes a global pipe instance for the current process.
HostGoldfishPipeDevice* getHostPipeInstance();

// connect, close, read, write and poll.
void* host_pipe_connect(const char* name);
void host_pipe_close(void* pipe);
ssize_t host_pipe_read(void* pipe, void* buffer, size_t len);
ssize_t host_pipe_write(void* pipe, const void* buffer, size_t len);
unsigned host_pipe_poll(void* pipe);

} // namespace android

extern "C" {

typedef void* (*host_pipe_connect_t)(const char*);
typedef void (*host_pipe_close_t)(void*);
typedef ssize_t (*host_pipe_read_t)(void*, void* , size_t);
typedef ssize_t (*host_pipe_write_t)(void* pipe, const void* buffer, size_t len);
typedef unsigned (*host_pipe_poll_t)(void* pipe);

struct HostPipeDispatch {
    host_pipe_connect_t connect = 0;
    host_pipe_close_t close = 0;
    host_pipe_read_t read = 0;
    host_pipe_write_t write = 0;
    host_pipe_poll_t poll = 0;
};

HostPipeDispatch make_host_pipe_dispatch();

}


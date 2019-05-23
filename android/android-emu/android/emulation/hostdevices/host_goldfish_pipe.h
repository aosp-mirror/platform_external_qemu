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

extern "C" {

// C interface for HostGoldfishPipe.

void android_host_pipe_device_initialize();
void* android_host_pipe_connect(const char* name);
void android_host_pipe_close(void* pipe);
ssize_t android_host_pipe_read(void* pipe, void* buffer, size_t len);
ssize_t android_host_pipe_write(void* pipe, const void* buffer, size_t len);
unsigned android_host_pipe_poll(void* pipe);

typedef void* (*android_host_pipe_connect_t)(const char*);
typedef void (*android_host_pipe_close_t)(void*);
typedef ssize_t (*android_host_pipe_read_t)(void*, void* , size_t);
typedef ssize_t (*android_host_pipe_write_t)(void* pipe, const void* buffer, size_t len);
typedef unsigned (*android_host_pipe_poll_t)(void* pipe);

struct android_host_pipe_dispatch {
    android_host_pipe_connect_t connect = 0;
    android_host_pipe_close_t close = 0;
    android_host_pipe_read_t read = 0;
    android_host_pipe_write_t write = 0;
    android_host_pipe_poll_t poll = 0;
};

android_host_pipe_dispatch make_host_pipe_dispatch();

} // extern "C"

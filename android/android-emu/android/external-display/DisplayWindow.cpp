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

#include "android/external-display/DisplayWindow.h"
#include "android/external-display/SDLRenderer.h"
#include "android/utils/debug.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <pthread.h>

namespace android {
namespace display {

DisplayWindow* DisplayWindow::create(int width, int height) {
    PRIVATE_DATA data = {0};

    VERBOSE_PRINT(
            extdisplay,
            "DisplayWindow::create(pid=%d, tid=%p): width=%d, height=%d\n",
            getpid(), (void*)pthread_self(), width, height);

    data.width = width;
    data.height = height;

    pthread_mutex_init(&data.mutex, NULL);
    pthread_cond_init(&data.cond, NULL);

    pthread_t thread = (pthread_t)NULL;
    pthread_create(&thread, NULL, sdl_thread_func, &data);

    if (thread != 0) {
        pthread_mutex_lock(&data.mutex);
        pthread_cond_wait(&data.cond, &data.mutex);
        pthread_mutex_unlock(&data.mutex);

        pthread_mutex_destroy(&data.mutex);
        pthread_cond_destroy(&data.cond);

        return data.wfi;
    } else {
        return nullptr;
    }
}

void DisplayWindow::resize(int width, int height) {
    SDLRenderer::resize_sdl_window(this, width, height);
}

void DisplayWindow::update(unsigned char* rgb,
                           int size,
                           int width,
                           int height) {}

void DisplayWindow::update_yuv(AVFrame* frame,
                    int y_data_size,
                    unsigned char* y_data,
                    int u_data_size,
                    unsigned char* u_data,
                    int v_data_size,
                    unsigned char* v_data,
                    int width,
                    int height) {}

void DisplayWindow::remove() {
    SDLRenderer::delete_sdl_window(this);
}

}  // namespace display
}  // namespace android

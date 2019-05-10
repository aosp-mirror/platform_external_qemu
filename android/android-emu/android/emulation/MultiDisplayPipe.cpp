/*
* Copyright (C) 2019 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "MultiDisplayPipe.h"
#include "../opengles.h"

#define ADD 1
#define DEL 2
#define ADDMSGLEN 21
#define DELMSGLEN 5

namespace android {

MultiDisplayPipe* sInstance = NULL;

MultiDisplayPipe::MultiDisplayPipe(AndroidPipe::Service* service, PipeArgs&& pipeArgs)
  : AndroidAsyncMessagePipe(service, std::move(pipeArgs)) {
    if (sInstance) {
        DLOG(ERROR) << "MultiDisplayPipe already created";
    }
    sInstance = this;
}

MultiDisplayPipe::~MultiDisplayPipe() {
    sInstance = NULL;
}

void MultiDisplayPipe::onMessage(const std::vector<uint8_t>& data) {
    // Expected data is {displayId, colorBufferId}
    if (data.size() != 8) {
        DLOG(ERROR) << "Wrong msg for MultiDisplayPipe cb binding";
        return;
    }
    uint32_t id = *(uint32_t*)&(data[0]);
    uint32_t cb = *(uint32_t*)&(data[4]);

    android_setMultiDisplayColorBuffer(id, cb);
}

void MultiDisplayPipe::addMultiDisplay(uint32_t id, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                                  uint32_t dpi, uint32_t flag) {
    std::vector<uint8_t> data;
    data.resize(ADDMSGLEN);
    uint8_t* p = data.data();
    *p = ADD;
    p++;
    memcpy(p, &id, 4);
    p+=4;
    memcpy(p, &w, 4);
    p+=4;
    memcpy(p, &h, 4);
    p+=4;
    memcpy(p, &dpi, 4);
    p+=4;
    memcpy(p, &flag, 4);
    p+=4;
    send(std::move(data));

    android_setMultiDisplay(id, x, y, w, h, dpi, true);
}

void MultiDisplayPipe::delMultiDisplay(uint32_t id) {
    std::vector<uint8_t> data;
    data.resize(DELMSGLEN);
    uint8_t* p = data.data();
    *p = DEL;
    p++;
    memcpy(p, &id, 4);
    send(std::move(data));

    android_setMultiDisplay(id, 0, 0, 0, 0, 0, false);
}

MultiDisplayPipe* MultiDisplayPipe::getPipe() {
    return sInstance;
}
} // namespace android

void android_init_multi_display_pipe() {
    android::AndroidPipe::Service::add(
        new android::AndroidAsyncMessagePipe::Service<android::MultiDisplayPipe>("multidisplay"));
}

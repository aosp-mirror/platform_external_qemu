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
#include "android/opengles.h"

#define ADD 1
#define DEL 2
#define CMD_SIZE 21

namespace android {

static MultiDisplayPipe* sMultiDisplayPipeInstance = nullptr;
static base::Lock mLock;
static std::vector<uint8_t> sData;

MultiDisplayPipe::MultiDisplayPipe(AndroidPipe::Service* service, PipeArgs&& pipeArgs)
  : AndroidAsyncMessagePipe(service, std::move(pipeArgs)) {
    if (sMultiDisplayPipeInstance) {
        LOG(ERROR) << "MultiDisplayPipe already created";
    }
    sMultiDisplayPipeInstance = this;
    {
        android::base::AutoLock lock(mLock);
        if (sData.size() == 0) {
            return;
        }
        for (int i = 0; i < sData.size(); i += CMD_SIZE) {
            std::vector<uint8_t> data(sData.begin() + i,
                                      sData.begin() + i + CMD_SIZE);
            LOG(VERBOSE) << "send buffered cmd " << data[0];
            send(std::move(data));
        }
        sData.clear();
    }
}

MultiDisplayPipe::~MultiDisplayPipe() {
    sMultiDisplayPipeInstance = NULL;
}

void MultiDisplayPipe::onMessage(const std::vector<uint8_t>& data) {
    // Expected data is {displayId, colorBufferId}
    if (data.size() != 8) {
        LOG(ERROR) << "Wrong msg for MultiDisplayPipe cb binding";
        return;
    }
    uint32_t id = *(uint32_t*)&(data[0]);
    uint32_t cb = *(uint32_t*)&(data[4]);

    android_setMultiDisplayColorBuffer(id, cb);
}

//static
void MultiDisplayPipe::setMultiDisplay(uint32_t id, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                                       uint32_t dpi, uint32_t flag, bool add) {
    if (sMultiDisplayPipeInstance) {
        std::vector<uint8_t> data;
        fillData(data, id, w, h, dpi, flag, add);
        sMultiDisplayPipeInstance->send(std::move(data));
    } else {
        //buffer the data, send it later when pipe connected
        android::base::AutoLock lock(mLock);
        fillData(sData, id, w, h, dpi, flag, add);
    }
    // adjust the host window
    android_setMultiDisplay(id, x, y, w, h, dpi, add);
}

void MultiDisplayPipe::fillData(std::vector<uint8_t>& data, uint32_t id, uint32_t w, uint32_t h,
                                uint32_t dpi, uint32_t flag, bool add) {
    uint32_t input[] = {id, w, h, dpi, flag};
    uint8_t* p = (uint8_t*)input;
    if (add) {
        data.push_back(ADD);
    } else {
        data.push_back(DEL);
    }
    for (int i = 0; i < sizeof(input); i++) {
        data.push_back(p[i]);
    }
}

} // namespace android

void android_init_multi_display_pipe() {
    android::AndroidPipe::Service::add(
        new android::AndroidAsyncMessagePipe::Service<android::MultiDisplayPipe>("multidisplay"));
}

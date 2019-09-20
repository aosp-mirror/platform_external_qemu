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

namespace android {

static MultiDisplayPipe* sMultiDisplayPipeInstance = nullptr;
const uint8_t MultiDisplayPipe::ADD = 1;
const uint8_t MultiDisplayPipe::DEL = 2;
const uint8_t MultiDisplayPipe::QUERY = 3;
const uint8_t MultiDisplayPipe::BIND = 4;
const uint8_t MultiDisplayPipe::MAX_DISPLAYS = 10;

MultiDisplayPipe::MultiDisplayPipe(AndroidPipe::Service* service, PipeArgs&& pipeArgs)
  : AndroidAsyncMessagePipe(service, std::move(pipeArgs)),
    mService(static_cast<Service*>(service)) {
    LOG(VERBOSE) << "MultiDisplayPipe created " << this;
    if (sMultiDisplayPipeInstance) {
        LOG(ERROR) << "MultiDisplayPipe already created";
    }
    sMultiDisplayPipeInstance = this;
}

MultiDisplayPipe::~MultiDisplayPipe() {
    LOG(VERBOSE) << "MultiDisplayPipe deleted self " << this;
    sMultiDisplayPipeInstance = NULL;
}

void MultiDisplayPipe::onMessage(const std::vector<uint8_t>& data) {
    uint8_t cmd = data[0];
    switch (cmd) {
        case QUERY:
            LOG(VERBOSE) << "MultiDisplayPipe recevied QUERY";
            for (uint32_t i = 1; i < MAX_DISPLAYS + 1; i++) {
                uint32_t w, h, dpi, flag;
                if (mService->mWindowAgent->getMultiDisplay(i, NULL, NULL, &w, &h,
                                                            &dpi, &flag, NULL)) {
                    if (dpi == 0) {
                        // Display created by guest through rcCommand,
                        // not by UI/config.ini/cmd, need not report
                        continue;
                    }
                    std::vector<uint8_t> buf;
                    fillData(buf, i, w, h, dpi, flag, true);
                    LOG(VERBOSE) << "MultiDisplayPipe send add id " << i << " width " << w
                                 << " height " << h << " dpi " << dpi << " flag " << flag;
                    send(std::move(buf));
                }
            }
            break;
        case BIND: {
            uint32_t id = *(uint32_t*)&(data[1]);
            uint32_t cb = *(uint32_t*)&(data[5]);
            LOG(VERBOSE) << "MultiDisplayPipe bind display " << id << " cb " << cb;
            android_setMultiDisplayColorBuffer(id, cb);
            break;
        }
        default:
            LOG(WARNING) << "unexpected cmommand " << cmd;
    }

}

//static
void MultiDisplayPipe::setMultiDisplay(uint32_t id,
                                       int32_t x,
                                       int32_t y,
                                       uint32_t w,
                                       uint32_t h,
                                       uint32_t dpi,
                                       uint32_t flag,
                                       bool add) {
    if (sMultiDisplayPipeInstance) {
        std::vector<uint8_t> data;
        fillData(data, id, w, h, dpi, flag, add);
        LOG(VERBOSE) << "MultiDisplayPipe send " << (add ? "add":"del") << " id " << id
                     << " width " << w << " height " << h << " dpi " << dpi
                     << " flag " << flag;
        sMultiDisplayPipeInstance->send(std::move(data));
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

void android_init_multi_display_pipe(const QAndroidEmulatorWindowAgent* const agent) {
    android::AndroidPipe::Service::add(
        new android::MultiDisplayPipe::Service("multidisplay", agent));
}


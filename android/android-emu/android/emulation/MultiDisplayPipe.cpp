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
#include "MultiDisplay.h"
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
        case QUERY: {
            LOG(VERBOSE) << "MultiDisplayPipe recevied QUERY";
            uint32_t w, h, dpi, flag, id;
            int32_t startId = -1;
            const auto ins = MultiDisplay::getInstance();
            while (ins && ins->getNextMultiDisplay(startId, &id, nullptr, nullptr,
                                                    &w, &h, &dpi, &flag, nullptr)) {
                if (id >= MultiDisplay::s_displayIdInternalBegin) {
                    // Display created by guest through rcCommand, e.g., HWC2,
                    // not by UI/config.ini/cmd, need not report
                    break;
                }
                std::vector<uint8_t> buf;
                fillData(buf, id, w, h, dpi, flag, true);
                LOG(VERBOSE) << "MultiDisplayPipe send add id " << id << " width " << w
                                << " height " << h << " dpi " << dpi << " flag " << flag;
                send(std::move(buf));
                startId = id;
            }
            break;
        }
        case BIND: {
            uint32_t id = *(uint32_t*)&(data[1]);
            uint32_t cb = *(uint32_t*)&(data[5]);
            LOG(VERBOSE) << "MultiDisplayPipe bind display " << id << " cb " << cb;
            const auto ins = MultiDisplay::getInstance();
            if (ins) {
                ins->setDisplayColorBuffer(id, cb);
            }
            break;
        }
        default:
            LOG(WARNING) << "unexpected cmommand " << cmd;
    }

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

void MultiDisplayPipe::onSave(base::Stream* stream) {
    AndroidAsyncMessagePipe::onSave(stream);
    MultiDisplay* instance = MultiDisplay::getInstance();
    if (!instance) {
        LOG(ERROR) << "Failed to save MultiDisplay info, MultiDisplay "
                   << "not initiated";
        return;
    }
    instance->onSave(stream);
}

void MultiDisplayPipe::onLoad(base::Stream* stream) {
    AndroidAsyncMessagePipe::onLoad(stream);
    MultiDisplay* instance = MultiDisplay::getInstance();
    if (!instance) {
        LOG(ERROR) << "Failed to load MultiDisplay info, MultiDisplay "
                   << "not initiated";
        return;
    }
    instance->onLoad(stream);
}

MultiDisplayPipe* MultiDisplayPipe::getInstance() {
    return sMultiDisplayPipeInstance;
}
} // namespace android

void android_init_multi_display_pipe() {
    android::AndroidPipe::Service::add(
        std::make_unique<android::MultiDisplayPipe::Service>("multidisplay"));
}


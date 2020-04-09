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

#pragma once

#include "AndroidAsyncMessagePipe.h"
#include "android/emulation/control/window_agent.h"

namespace android {
class MultiDisplayPipe : public AndroidAsyncMessagePipe {
public:
    class Service : public AndroidAsyncMessagePipe::Service<MultiDisplayPipe> {
    public:
        Service(const char* serviceName)
          : AndroidAsyncMessagePipe::Service<MultiDisplayPipe>(serviceName)
        {}
    };

    MultiDisplayPipe(AndroidPipe::Service* service, PipeArgs&& pipeArgs);
    virtual ~MultiDisplayPipe();

    void onMessage(const std::vector<uint8_t>& data) override;
    void fillData(std::vector<uint8_t>& data, uint32_t id, uint32_t w, uint32_t h,
                  uint32_t dpi, uint32_t flag, bool add);
    virtual void onSave(base::Stream* stream) override;
    virtual void onLoad(base::Stream* stream) override;

    static MultiDisplayPipe* getInstance();
    const static uint8_t ADD;
    const static uint8_t DEL;
    const static uint8_t QUERY;
    const static uint8_t BIND;
    const static uint8_t MAX_DISPLAYS;

private:
    Service* const mService;
};
}

void android_init_multi_display_pipe();

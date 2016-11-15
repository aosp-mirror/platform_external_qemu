// Copyright (C) 2016 The Android Open Source Project
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

#include "OpenglRender/IOStream.h"
#include "RenderChannelImpl.h"

#include <memory>
#include <vector>

namespace emugl {

class ChannelStream final : public IOStream {
public:
    ChannelStream(std::shared_ptr<RenderChannelImpl> channel,
                  size_t bufSize);

    virtual void* allocBuffer(size_t minSize) override final;
    virtual int commitBuffer(size_t size) override final;
    virtual const unsigned char* read(void* buf,
                                      size_t* inout_len) override final;

    void forceStop();

private:
    std::shared_ptr<RenderChannelImpl> mChannel;
    RenderChannel::Buffer mBuf;
};

}  // namespace emugl

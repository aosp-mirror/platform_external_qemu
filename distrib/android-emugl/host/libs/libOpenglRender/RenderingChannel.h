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

#include "OpenglRender/IRenderingChannel.h"
#include "Renderer.h"

#include "android/base/Compiler.h"

#include <memory>

namespace emugl {

class RenderingChannel final : public IRenderingChannel {
public:
    RenderingChannel(std::shared_ptr<Renderer> renderer);
    ~RenderingChannel();

public:
    // IRenderingChannel implementation, operations provided for a guest system
    virtual void setOperationReadyCallback(
            OperationReadyCallback callback) override final;

    virtual bool write(ChannelBuffer&& buffer) override final;
    virtual ChannelBuffer read() override final;

    virtual void stop() override final;

public:
    bool writeToGuest(const ChannelBuffer::value_type* buf, size_t size);
    size_t readFromGuest(ChannelBuffer::value_type* buf, size_t size);

private:
    DISALLOW_COPY_ASSIGN_AND_MOVE(RenderingChannel);

private:
    std::shared_ptr<Renderer> mRenderer;
};

}  // namespace emugl

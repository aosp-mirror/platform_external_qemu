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
#include "RenderingChannel.h"

namespace emugl {

RenderingChannel::RenderingChannel(std::shared_ptr<Renderer> renderer)
    : mRenderer(renderer) {}

RenderingChannel::~RenderingChannel() {}

void RenderingChannel::setEventCallback(IRenderingChannel::EventCallback callback) {}

bool RenderingChannel::write(ChannelBuffer&& buffer) {
    return true;
}

ChannelBuffer RenderingChannel::read() {
    return {};
}

void RenderingChannel::stop() {
    return;
}

bool RenderingChannel::writeToGuest(const ChannelBuffer::value_type* buf,
                                    size_t size) {
    return true;
}

size_t RenderingChannel::readFromGuest(ChannelBuffer::value_type* buf,
                                       size_t size) {
    return size;
}

}  // namespace emugl

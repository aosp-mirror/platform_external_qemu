/*
* Copyright (C) 2011 The Android Open Source Project
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

#include "emugl/common/mutex.h"
#include "emugl/common/thread.h"

#include <memory>

namespace emugl {

class RenderChannelImpl;
class RendererImpl;

// A class used to model a thread of the RenderServer. Each one of them
// handles a single guest client / protocol byte stream.
class RenderThread : public emugl::Thread {
public:
    // Create a new RenderThread instance.
    static std::unique_ptr<RenderThread> create(
            std::weak_ptr<RendererImpl> renderer,
            std::shared_ptr<RenderChannelImpl> channel);

    virtual ~RenderThread();

    // Returns true iff the thread has finished.
    // Note that this also means that the thread's stack has been
    bool isFinished() { return tryWait(NULL); }

private:
    RenderThread(std::weak_ptr<RendererImpl> renderer,
                 std::shared_ptr<RenderChannelImpl> channel);

    virtual intptr_t main();

    std::shared_ptr<RenderChannelImpl> mChannel;
    std::weak_ptr<RendererImpl> mRenderer;
};

}  // namespace emugl

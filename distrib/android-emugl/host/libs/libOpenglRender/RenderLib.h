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

#include "OpenglRender/IRenderLib.h"

#include "android/base/Compiler.h"

#include <memory>

namespace emugl {

class RenderLib final : public IRenderLib,
                        public std::enable_shared_from_this<RenderLib> {
public:
    RenderLib() = default;

    virtual void setLogger(emugl_logger_struct logger) override;
    virtual void setCrashReporter(crash_reporter_t reporter) override;

    virtual IRendererPtr initRenderer(int width,
                                      int height,
                                      bool useSubWindow) override;

private:
    DISALLOW_COPY_ASSIGN_AND_MOVE(RenderLib);

private:
    std::weak_ptr<IRenderer> mRenderer;
};

}  // namespace emugl

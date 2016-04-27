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

#include <memory>

#include "OpenglRender/IRenderer.h"
#include "OpenglRender/render_api_types.h"

namespace emugl {

class IRenderLib {
public:
    virtual void setLogger(emugl_logger_struct logger) = 0;
    virtual void setCrashReporter(crash_reporter_t reporter) = 0;

    virtual IRendererPtr initRenderer(int width, int height, bool useSubWindow) = 0;

protected:
    ~IRenderLib() = default;
};

using IRenderLibPtr = std::shared_ptr<IRenderLib>;

}  // namespace emugl

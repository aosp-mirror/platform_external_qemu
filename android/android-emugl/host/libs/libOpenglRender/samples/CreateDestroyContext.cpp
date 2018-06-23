// Copyright (C) 2018 The Android Open Source Project
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

#include "android/base/system/System.h"

#include "Standalone.h"

#include <functional>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using android::base::System;

namespace emugl {

class CreateDestroyContext : public SampleApplication {
public:
    CreateDestroyContext() : SampleApplication(256, 256, 60) { }
protected:

    struct VertexAttributes {
        float position[2];
        float color[3];
    };

    void initialize() override {
    }

    void draw() override {
        FrameBuffer* fb = FrameBuffer::getFB();

        unsigned int colorBuffer = fb->createColorBuffer(256, 256, GL_RGBA, FRAMEWORK_FORMAT_GL_COMPATIBLE);
        fb->openColorBuffer(colorBuffer);

        unsigned int context = fb->createRenderContext(0, 0, GLESApi_3_0);
        unsigned int surface = fb->createWindowSurface(0, 256, 256);

        fb->bindContext(context, surface, surface);

        fb->setWindowSurfaceColorBuffer(surface, colorBuffer);

        mTime += 0.05f;

        fb->flushWindowSurfaceColorBuffer(surface);

        fb->bindContext(0, 0, 0);
        fb->DestroyWindowSurface(surface);
        fb->closeColorBuffer(colorBuffer);
        fb->closeColorBuffer(colorBuffer);
        fb->DestroyRenderContext(context);
        this->rebind();
    }

private:
    GLint mTransformLoc;
    GLuint mBuffer;
    float mTime = 0.0f;
};

} // namespace emugl

int main(int argc, char** argv) {

    emugl::CreateDestroyContext app;
    app.drawLoop();

    return 0;
}

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

#include "GLSnapshotSampleAppTesting.h"
#include "../samples/HelloTriangle.h"

#include <gtest/gtest.h>

namespace emugl {

class HelloTriangleSnapshotTest : public HelloTriangle {
    void initialize() override {
        HelloTriangle::initialize();
    }

    void draw() override {
        // TODO(benzene): this function
        // do a snapshot
        fprintf(stderr, "YAES overrided %s\n", __func__);
        HelloTriangle::draw();
        // save pixels

        // undo non-GL state change somehow

        // load the snapshot
        HelloTriangle::draw();
        // compare pixels
    }
};

TEST(SnapshotGlSampleApplication, InstantiateApp) {
    fprintf(stderr, "Starting test %s\n", __func__);
    HelloTriangleSnapshotTest app;
    app.drawLoop();
}

}  // namespace emugl

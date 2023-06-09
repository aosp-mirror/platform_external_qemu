// Copyright (C) 2023 The Android Open Source Project
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

#include <QFrame>  // for QFrame
#include "host-common/qt_ui_defs.h"

class VirtualSceneControlWindow;
class VirtualSensorsPage;

class ExtendedBaseWindow : public QFrame {

public:
    ExtendedBaseWindow() : QFrame(nullptr){};
    virtual ~ExtendedBaseWindow() = default;

    virtual void sendMetricsOnShutDown() = 0;

    virtual void show() = 0;
    virtual void showPane(ExtendedWindowPane pane) = 0;

    // Wait until this component has reached the visibility
    // state.
    virtual void waitForVisibility(bool visible) = 0;

    virtual void connectVirtualSceneWindow(
            VirtualSceneControlWindow* virtualSceneWindow) = 0;

    // TODO(jansene): This needs to be generalized to handle gRPC
    virtual VirtualSensorsPage* getVirtualSensorsPage() = 0;
};
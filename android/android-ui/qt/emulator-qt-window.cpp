/* Copyright (C) 2020 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#include "emulator-qt-window.h"

using namespace android::emulation::control;
using ScreenshotRequest =
        android::emulation::control::EmulatorControllerClient::ScreenshotRequest;

EmulatorQtWindow::EmulatorQtWindow(std::shared_ptr<grpc::Channel> channel, QWidget* parent) :
        emu_client_(new EmulatorControllerClient(channel)),
        QFrame(parent) {
    ImageFormat image_format;
    image_format.set_format(ImageFormat::RGBA8888);
    image_format.set_width(1080);
    image_format.set_height(1920);
    image_format.set_display(0);

    emu_client_->StreamScreenshotAsync(image_format, [this](Image i) -> ScreenshotRequest {
        return ShowScreenshot(i);
    });
}

// Callback passed into the emulator gRPC client.
ScreenshotRequest EmulatorQtWindow::ShowScreenshot(android::emulation::control::Image image) {
    return ScreenshotRequest::More;
}

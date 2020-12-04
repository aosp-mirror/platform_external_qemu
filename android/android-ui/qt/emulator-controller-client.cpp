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

#include "emulator-controller-client.h"

using namespace android::emulation::control;

EmulatorControllerClient::EmulatorControllerClient(std::shared_ptr<grpc::Channel> channel) :
            stub_(EmulatorController::NewStub(channel)) {
}

void EmulatorControllerClient::StreamScreenshotAsync(ImageFormat format, ScreenshotCallback cb) {
    // TODO: stop/restart on consecutive calls to StreamScreenshotAsync
    stream_screenshot_thread_ = std::thread([=]() {
        grpc::ClientContext context;
        Image image;

        std::unique_ptr<grpc::ClientReader<Image>> reader(
                stub_->streamScreenshot(&context, format));
        while (reader->Read(&image)) {
            std::cout << "Got new screenshot seq=" << image.seq()
                      << " width=" << image.format().width()
                      << " height=" << image.format().height();
            switch (cb(image)) {
                case ScreenshotRequest::More:
                    continue;
                case ScreenshotRequest::Stop:
                default:
                    std::cout << "Stopping screenshot stream";
                    return;
            }
        }

        auto status = reader->Finish();
        if (status.ok()) {
            std::cout << "Emulator stopped sending screenshots.";
        } else {
            std::cout << "StreamScreenshot failed during a read";
        }
    });
}

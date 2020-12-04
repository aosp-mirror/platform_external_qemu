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

#pragma once

#include <thread>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "emulator_controller.grpc.pb.h"
#include "emulator_controller.pb.h"

namespace android {
namespace emulation {
namespace control {

class EmulatorControllerClient {
public:
    explicit EmulatorControllerClient(std::shared_ptr<grpc::Channel> channel);
    virtual ~EmulatorControllerClient();

    // Client's mechanism to continue/stop the screenshot screen. Return it in the callback.
    enum class ScreenshotRequest {
        More = 0,
        Stop,
    };
    using ScreenshotCallback = std::function<ScreenshotRequest(const Image&)>;
    void StreamScreenshotAsync(ImageFormat format, ScreenshotCallback cb);

private:
    std::unique_ptr<EmulatorController::Stub> stub_;
    std::thread stream_screenshot_thread_;
};  // EmulatorControllerClient

}  // namespace control
}  // namespace emulation
}  // namespace android

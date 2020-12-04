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

#define DEBUG 1
#if DEBUG
#define D(...) fprintf(stderr, __VA_ARGS__)
#else
#define D(...) \
    do {       \
    } while (0)
#endif

using namespace android::emulation::control;

EmulatorControllerClient::EmulatorControllerClient(
        std::shared_ptr<grpc::Channel> channel)
    : stub_(EmulatorController::NewStub(channel)) {
}

EmulatorControllerClient::~EmulatorControllerClient() {
    if (stream_screenshot_thread_.joinable()) {
        stream_screenshot_thread_.join();
    }
}

void EmulatorControllerClient::StreamScreenshotAsync(ImageFormat format, ScreenshotCallback cb) {
    // TODO: stop/restart on consecutive calls to StreamScreenshotAsync
    stream_screenshot_thread_ = std::thread([this, format, cb]() {
        grpc::ClientContext context;
        Image image;
        grpc::ChannelArguments ch_args;
        ch_args.SetMaxReceiveMessageSize(-1);
        std::shared_ptr<grpc::Channel> channel = grpc::CreateCustomChannel(
                "localhost:12345", grpc::InsecureChannelCredentials(), ch_args);

        auto waitUntil = gpr_time_add(gpr_now(GPR_CLOCK_REALTIME),
                                      gpr_time_from_seconds(5, GPR_TIMESPAN));
        bool connect = channel->WaitForConnected(waitUntil);
        auto state = channel->GetState(true);

        auto stub = EmulatorController::NewStub(channel);
        if (state == GRPC_CHANNEL_READY || state == GRPC_CHANNEL_IDLE) {
            D("GRPC ready\n");
        } else {
            D("GRPC not ready\n");
        }

        std::unique_ptr<grpc::ClientReader<Image>> reader =
                stub->streamScreenshot(&context, format);
        if (!reader) {
            D("Bad news bear!");
        }

        D("Waiting for first screenshot...\n");
        while (reader->Read(&image)) {
            D("Got screenshot!\n");
            std::cout << "Got new screenshot seq=" << image.seq()
                      << " width=" << image.format().width()
                      << " height=" << image.format().height() << std::endl;
            switch (cb(image)) {
                case ScreenshotRequest::More:
                    continue;
                case ScreenshotRequest::Stop:
                default:
                    std::cout << "Stopping screenshot stream\n";
                    return;
            }
        }

        auto status = reader->Finish();
        if (status.ok()) {
            std::cout << "Emulator stopped sending screenshots.\n";
        } else {
            std::cout << "StreamScreenshot failed during a read\n";
        }
    });
}

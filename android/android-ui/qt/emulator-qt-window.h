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

#include <grpcpp/channel.h>
#include <QFrame>
#include <QWidget>

#include "emulator-controller-client.h"

class EmulatorQtWindow final : public QFrame {
    Q_OBJECT

public:
    explicit EmulatorQtWindow(
            std::shared_ptr<grpc::Channel> channel,
            QWidget* parent = 0);
    virtual ~EmulatorQtWindow();

    void paintEvent(QPaintEvent* e) override;

private:
    using ScreenshotRequest =
            android::emulation::control::EmulatorControllerClient::ScreenshotRequest;
    ScreenshotRequest ShowScreenshot(const android::emulation::control::Image& image);

    std::unique_ptr<android::emulation::control::EmulatorControllerClient> emu_client_;
    // TODO: We probably need some sync stuff here + multiple pixmaps for concurrent read/write
    QPixmap pixmap_;
    const uint8_t* img_data_ = nullptr;
    int mmap_fd_ = -1;
    void* mmap_ptr = nullptr;
};

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

#include <QPainter>

#define DEBUG 1
#if DEBUG
#define D(...) fprintf(stderr, __VA_ARGS__)
#else
#define D(...) do { } while (0)
#endif

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

    resize(1080, 1920);
    show();
    emu_client_->StreamScreenshotAsync(image_format, [this](Image i) -> ScreenshotRequest {
        return ShowScreenshot(i);
    });
}

void EmulatorQtWindow::paintEvent(QPaintEvent* e) {
    QPainter painter(this);
    painter.drawPixmap(0, 0, 1080, 1920, pixmap_);
}

// Callback passed into the emulator gRPC client.
ScreenshotRequest EmulatorQtWindow::ShowScreenshot(android::emulation::control::Image image) {
    D("%s: w=%u h=%u seq=%u", __func__,
            image.format().width(), image.format().height(), image.seq());
    if (pixmap_.loadFromData(reinterpret_cast<const unsigned char*>(image.image().data()),
                image.format().width() * image.format().height() * 4, "RGBA8888")) {
        repaint();
    } else {
        D("Failed to load pixmap data");
    }
    return ScreenshotRequest::More;
}

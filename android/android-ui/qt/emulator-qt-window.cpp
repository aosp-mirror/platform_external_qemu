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

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define DEBUG 1
#if DEBUG
#define D(...) fprintf(stderr, __VA_ARGS__)
#else
#define D(...) do { } while (0)
#endif

#define IMG_WIDTH 1080 / 3
#define IMG_HEIGHT 1920 / 3

#if defined(Q_OS_LINUX) || defined(__APPLE__)
#define MMAP_FILE "/var/tmp/emu-client-12345.mmap"
#elif _WIN32
#define MMAP_FILE "/var/tmp/emu-client-12345.mmap"
#else
#error "Unknown OS! Where should tmp mmap file go?"
#endif

constexpr bool g_use_mmap = true;

using namespace android::emulation::control;
using ScreenshotRequest =
        android::emulation::control::EmulatorControllerClient::ScreenshotRequest;

EmulatorQtWindow::EmulatorQtWindow(std::shared_ptr<grpc::Channel> channel, QWidget* parent) :
        emu_client_(new EmulatorControllerClient(channel)),
        QFrame(parent) {
    ImageFormat image_format;
    image_format.set_format(ImageFormat::RGB888);
    image_format.set_width(IMG_WIDTH);
    image_format.set_height(IMG_HEIGHT);
    image_format.set_display(0);
    if (g_use_mmap) {
        D("Trying to use mmap file %s\n", MMAP_FILE);
        mmap_fd_ = open(MMAP_FILE, O_RDWR | O_CREAT, 0666);
        if (mmap_fd_ < 0) {
            perror("FATAL: unable to open file for mmap\n");
            exit(1);
        }
        if (ftruncate(mmap_fd_, IMG_WIDTH * IMG_HEIGHT * 3) == -1) {
            perror("Failed to increase size of mmaped file");
            exit(1);
        }

        mmap_ptr = mmap(NULL, IMG_WIDTH * IMG_HEIGHT * 3, PROT_READ | PROT_WRITE, MAP_SHARED,
                mmap_fd_, 0);
        if (mmap_ptr == MAP_FAILED) {
            perror("FATAL: mmap failed\n");
            exit(1);
        }
        image_format.mutable_transport()->set_channel(ImageTransport::MMAP);

        std::string url = "file:///";
        url += MMAP_FILE;
        image_format.mutable_transport()->set_handle(url);
        D("Using mmap file for screenshot stream [%s]\n", MMAP_FILE);
    }

    resize(IMG_WIDTH, IMG_HEIGHT);
    show();
    emu_client_->StreamScreenshotAsync(image_format, [this](const Image& i) -> ScreenshotRequest {
        return ShowScreenshot(i);
    });
}

EmulatorQtWindow::~EmulatorQtWindow() {
    if (mmap_fd_ < 0) {
        return;
    }
    munmap(mmap_ptr, IMG_WIDTH * IMG_HEIGHT * 3);
    ::close(mmap_fd_);
    ::remove(MMAP_FILE);
}

void EmulatorQtWindow::paintEvent(QPaintEvent* e) {
    QPainter painter(this);
    QImage image((g_use_mmap ? reinterpret_cast<uint8_t*>(mmap_ptr) : img_data_),
            IMG_WIDTH, IMG_HEIGHT, QImage::Format_RGB888);
    painter.drawImage(0, 0, image);
}

// Callback passed into the emulator gRPC client.
ScreenshotRequest EmulatorQtWindow::ShowScreenshot(const android::emulation::control::Image& image) {
    D("%s: w=%u h=%u seq=%u\n", __func__,
            image.format().width(), image.format().height(), image.seq());
    // TODO: should buffer the images in the gRPC client code and remove as we render them.
    img_data_ = reinterpret_cast<const unsigned char*>(image.image().data());
    repaint();
    return ScreenshotRequest::More;
}

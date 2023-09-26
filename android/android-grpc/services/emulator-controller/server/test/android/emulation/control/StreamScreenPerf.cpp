// Copyright (C) 2022 The Android Open Source Project
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

#include <assert.h>         // for assert
#include <grpcpp/grpcpp.h>  // for Clie...
#include <chrono>           // for oper...
#include <cstdint>          // for uint8_t
#include <memory>           // for uniq...
#include <string>           // for oper...
#include <thread>           // for slee...

#include "aemu/base/Log.h"                                    // for base

#include "aemu/base/files/PathUtils.h"                        // for Path...
#include "aemu/base/memory/SharedMemory.h"                    // for Shar...
#include "android/base/testing/TestTempDir.h"                    // for Test...
#include "android/emulation/control/utils/EmulatorGrcpClient.h"  // for Emul...
#include "android/utils/debug.h"                                 // for dprint
#include "benchmark/benchmark.h"
#include "emulator_controller.grpc.pb.h"                         // for Emul...
#include "emulator_controller.pb.h"                              // for Imag...
#include "google/protobuf/empty.pb.h"                            // for Empty

using namespace android::base;
using namespace android::emulation::control;
using ::google::protobuf::Empty;

// #define DEBUG 1
#ifndef DEBUG
#define DD(...) (void)0
#else
#define DD(fmt, ...) \
    dinfo("perf: %s:%d| " fmt, __func__, __LINE__, ##__VA_ARGS__)
#endif

// This contains a set of perfomance tests to measure how quickly we can detect
// a pixel change. The idea is that you will:
//
// Launch the emulator with the following parameters:
// emulator @My_AVD -grpc 8554 -share-vid
//
// Install AnimateBox
// (emu-master-dev/external/adt-infra/pytest/test_embedded/AnimateBox)
// ./gradlew installDebug
//
// Launch the app
//
// Next you can run the benchmark.
//
// The benchmark does the following:
//
// get_the_screen_size of the emulator
// loop:
//  send_click_to_change_pixel_color
//  wait_for_pixel_color_change
//
// The test attempts various ways to retrieve pixels and observe changes.
//
// The other set of tests just measure the time delay between frames.
// (Note: this might not be very useful.)

#define BASIC_BENCHMARK_TEST(x) \
    BENCHMARK(x)->UseRealTime()->Threads(1)->UseRealTime();

const std::chrono::microseconds kPollInterval = std::chrono::microseconds(100);

// Gets the center pixel. The pixel will always be 4 bytes.
inline uint32_t getCenterPixel(const ImageFormat& format, uint8_t* pixels) {
    assert(format.width() % 2 == 0);
    assert(format.height() % 2 == 0);
    assert(format.format() == ImageFormat::RGB888 ||
           format.format() == ImageFormat::RGBA8888);

    // First we calculate the location of the pixel;
    int bpp = format.format() == ImageFormat::RGBA8888 ? 4 : 3;

    uint32_t offset = format.width() * format.height() / 2 * bpp +
                      (format.width() / 2 * bpp);

    uint32_t pixel = pixels[offset] | (pixels[offset + 1] << 8) |
                     (pixels[offset + 2] << 16) | (pixels[offset + 3] << 24);

    return pixel;
}

uint32_t getCenterPixel(const Image& img, uint8_t* shared_mem = nullptr) {
    uint8_t* pixels = shared_mem;
    if (shared_mem == nullptr)
        pixels = (uint8_t*)img.image().data();
    return getCenterPixel(img.format(), pixels);
}

// Polls for the center pixel to change. A poll will sleep for a few usecs
// before trying again.
uint32_t pollForPixelChange(const ImageFormat format,
                            uint32_t current,
                            uint8_t* pixels) {
    // First we calculate the location of the pixel;
    int max_loop = 10000;
    int bpp = format.format() == ImageFormat::RGBA8888 ? 4 : 3;
    uint32_t pixel = current;
    do {
        pixel = getCenterPixel(format, pixels);
        if (pixel == current) {
            std::this_thread::sleep_for(kPollInterval);
        }
    } while (pixel == current && max_loop-- > 0);

    return pixel;
}

std::unique_ptr<EmulatorGrpcClient> sclient = nullptr;

EmulatorGrpcClient* getClient() {
    if (!sclient) {
        Endpoint destination;
        destination.set_target("localhost:8554");
        EmulatorGrpcClient::Builder builder;
        sclient = builder.withEndpoint(destination).build().value();
    }

    if (!sclient->hasOpenChannel()) {
        dfatal("Unable to open gRPC channel to the emulator.");
    }

    return sclient.get();
}

auto controllerStub() {
    return getClient()->stub<android::emulation::control::EmulatorController>();
}

Image startImage(ImageFormat_ImgFormat format) {
    auto ctx = getClient()->newContext();
    ctx->set_deadline(std::chrono::system_clock::now() +
                      std::chrono::seconds(1));

    ImageFormat fmt;
    fmt.set_format(format);
    Image img;
    auto status = controllerStub()->getScreenshot(ctx.get(), fmt, &img);
    assert(status.ok());
    return img;
}

// Sends a mouse click to the emulator.
void sendClick() {
    auto ctx = getClient()->newContext();
    ctx->set_deadline(std::chrono::system_clock::now() +
                      std::chrono::milliseconds(120));

    MouseEvent click;
    Empty empty;

    click.set_x(10);
    click.set_y(10);
    click.set_buttons(1);
    auto status = controllerStub()->sendMouse(ctx.get(), click, &empty);
    assert(status.ok());

    ctx = getClient()->newContext();
    ctx->set_deadline(std::chrono::system_clock::now() +
                      std::chrono::milliseconds(120));

    click.set_buttons(0);
    status = controllerStub()->sendMouse(ctx.get(), click, &empty);
    assert(status.ok());
}

void grpc_observe_click_change(Image start,
                               uint8_t* shared_mem,
                               benchmark::State& state) {
    auto ctx = getClient()->newContext();
    auto stub = controllerStub();
    ctx->set_deadline(std::chrono::system_clock::now() +
                      std::chrono::seconds(120));

    uint32_t currentPixel = getCenterPixel(start, shared_mem);

    DD("Start frame: %d, pixel: %x", start.seq(), currentPixel);

    uint32_t pixel = currentPixel;
    Image response;
    int oldSeq = 0;

    auto reader = stub->streamScreenshot(ctx.get(), start.format());
    while (state.KeepRunning()) {
        // send a click to force a new pixel.
        sendClick();

        // Keep requesting frames until we have a new pixel color.
        while (pixel == currentPixel && reader->Read(&response)) {
            pixel = getCenterPixel(response, shared_mem);
        }
        DD("Frame: %d, pixel: %x != %x", response.seq() - oldSeq, pixel,
           currentPixel);
        oldSeq = response.seq();
        currentPixel = pixel;
    }
}

void benchmark_frame_counter(Image img,
                             uint8_t* shared_mem,
                             benchmark::State& state) {
    auto ctx = getClient()->newContext();
    auto stub = controllerStub();
    ctx->set_deadline(std::chrono::system_clock::now() +
                      std::chrono::seconds(120));
    auto reader = stub->streamScreenshot(ctx.get(), img.format());
    Image response;
    while (state.KeepRunning()) {
        reader->Read(&response);
    }
}

void BM_stream_screen_rgba8888(benchmark::State& state) {
    grpc_observe_click_change(startImage(ImageFormat::RGBA8888), nullptr,
                              state);
}

void BM_stream_screen_rgb888(benchmark::State& state) {
    grpc_observe_click_change(startImage(ImageFormat::RGB888), nullptr, state);
}

void BM_stream_screen_shared_rgba8888(benchmark::State& state) {
    auto img = startImage(ImageFormat::RGBA8888);
    int size = img.format().width() * img.format().height() * 4;
    TestTempDir tempDir{"shared"};
    auto handle = "file://" +
                  android::base::PathUtils::join(tempDir.path(), "shared.mem");
    auto writer = std::make_unique<SharedMemory>(handle, size);
    auto user_read_only = 0600;
    int err = writer->create(user_read_only);

    auto format = img.mutable_format();
    ImageTransport transport;
    transport.set_channel(ImageTransport::MMAP);
    transport.set_handle(handle);
    format->mutable_transport()->set_handle(handle);
    format->mutable_transport()->set_channel(ImageTransport::MMAP);
    format->set_format(ImageFormat::RGBA8888);

    grpc_observe_click_change(img, (uint8_t*)writer->get(), state);
}


struct VideoInfo {
    uint32_t width;
    uint32_t height;
    uint32_t fps;          // Target framerate.
    uint32_t frameNumber;  // Frames are ordered
    uint64_t tsUs;         // Timestamp when this frame was received.
};

void BM_share_vid(benchmark::State& state) {
    auto img = startImage(ImageFormat::RGBA8888);
    VideoInfo* video = {0};
    int size = img.format().width() * img.format().height() * 4;

    auto handle = "videmulator5554";
    auto writer =
            std::make_unique<SharedMemory>(handle, size + sizeof(VideoInfo));
    auto user_read_only = 0600;
    int err = writer->open(SharedMemory::AccessMode::READ_ONLY);

    uint8_t* shared_mem = (uint8_t*)writer->get() + sizeof(VideoInfo);
    uint32_t currentPixel = getCenterPixel(img, shared_mem);

    video = (VideoInfo*)writer->get();

    int previousFrame = video->frameNumber;
    while (state.KeepRunning()) {
        sendClick();
        currentPixel =
                pollForPixelChange(img.format(), currentPixel, shared_mem);
        DD("Frame: %d, %x", video->frameNumber - previousFrame, currentPixel);
        previousFrame = video->frameNumber;
    }
}

void BM_send_click(benchmark::State& state) {
    while (state.KeepRunning()) {
        sendClick();
    }
}

void BM_share_vid_frame_counter(benchmark::State& state) {
    auto img = startImage(ImageFormat::RGBA8888);
    VideoInfo* video = {0};
    int size = img.format().width() * img.format().height() * 4;

    auto handle = "videmulator5554";
    auto writer =
            std::make_unique<SharedMemory>(handle, size + sizeof(VideoInfo));
    auto user_read_only = 0600;
    int err = writer->open(SharedMemory::AccessMode::READ_ONLY);
    video = (VideoInfo*)writer->get();
    int i = 0;
    int previousFrame = 0;
    while (state.KeepRunning()) {
        while (previousFrame == video->frameNumber) {
            /* spin */
            std::this_thread::sleep_for(kPollInterval);
        };
        previousFrame = video->frameNumber;
    }
}

void BM_stream_screen_rgba8888_frame_counter(benchmark::State& state) {
    auto img = startImage(ImageFormat::RGBA8888);
    benchmark_frame_counter(img, nullptr, state);
}

void BM_stream_screen_rgb888_frame_counter(benchmark::State& state) {
    auto img = startImage(ImageFormat::RGBA8888);
    benchmark_frame_counter(img, nullptr, state);
}

void BM_stream_screen_shared_rgba8888_frame_counter(benchmark::State& state) {
    auto img = startImage(ImageFormat::RGBA8888);
    int size = img.format().width() * img.format().height() * 4;
    TestTempDir tempDir{"shared"};
    auto handle = "file://" +
                  android::base::PathUtils::join(tempDir.path(), "shared.mem");
    auto writer = std::make_unique<SharedMemory>(handle, size);
    auto user_read_only = 0600;
    int err = writer->create(user_read_only);

    auto format = img.mutable_format();
    ImageTransport transport;
    transport.set_channel(ImageTransport::MMAP);
    transport.set_handle(handle);
    format->mutable_transport()->set_handle(handle);
    format->mutable_transport()->set_channel(ImageTransport::MMAP);
    format->set_format(ImageFormat::RGBA8888);

    benchmark_frame_counter(img, (uint8_t*)writer->get(), state);
}

BASIC_BENCHMARK_TEST(BM_send_click);
BASIC_BENCHMARK_TEST(BM_stream_screen_rgba8888);
BASIC_BENCHMARK_TEST(BM_stream_screen_rgb888);
BASIC_BENCHMARK_TEST(BM_stream_screen_shared_rgba8888);
BASIC_BENCHMARK_TEST(BM_share_vid);

BASIC_BENCHMARK_TEST(BM_stream_screen_rgba8888_frame_counter);
BASIC_BENCHMARK_TEST(BM_stream_screen_rgb888_frame_counter);
BASIC_BENCHMARK_TEST(BM_stream_screen_shared_rgba8888_frame_counter);
BASIC_BENCHMARK_TEST(BM_share_vid_frame_counter);
BENCHMARK_MAIN();

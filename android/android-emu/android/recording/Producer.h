// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

///
// Producer.h
//
// The definition of a producer. This will make it easier to swap out the
// audio/video producers for unit-testing purposes.
//

#pragma once

#include "android/base/threads/Thread.h"
#include "android/recording/Frame.h"

namespace android {
namespace recording {

class Producer : public android::base::Thread {
public:
    virtual ~Producer(){};

    // Callback to pass the contents of a new audio/video frame.
    using Callback = std::function<bool(const Frame* frame)>;

    // Attach the callback used to receiving new Frames
    void attachCallback(Callback cb) { mCallback = std::move(cb); }

    // Get the format of the audio or video data, whichever the producer is.
    AVFormat getFormat() { return mFormat; }

    // Stops the producer
    virtual void stop() = 0;

protected:
    Producer() = default;
    Producer(Producer&& p) = delete;

protected:
    Callback mCallback;
    AVFormat mFormat;
};

}  // namespace recording
}  // namespace android

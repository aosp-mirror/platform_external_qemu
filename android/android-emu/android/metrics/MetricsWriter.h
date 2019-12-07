// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include <memory>                   // for shared_ptr
#include <string>                   // for string, basic_string
#include <unordered_set>            // for unordered_set

#include "android/base/Compiler.h"  // for DISALLOW_COPY_ASSIGN_AND_MOVE

namespace wireless_android_play_playlog {
class LogEvent;
}

namespace android_studio {
class AndroidStudioEvent;
}

namespace android {
namespace metrics {

//
// MetricsWriter - an interface for writing metrics _somewhere_.
//
// This class abstracts out the metrics storage from the generic code that
// fills in the message and manages the reporting logic.
//
// To write a message, call write() with a pre-filled protobuf. This class
// doesn't modify the passed |event|, but just stores it in an implementation-
// defined format.
//

class MetricsWriter {
    DISALLOW_COPY_ASSIGN_AND_MOVE(MetricsWriter);

public:
    using Ptr = std::shared_ptr<MetricsWriter>;
    using AbandonedSessions = std::unordered_set<std::string>;

    virtual ~MetricsWriter();
    virtual void write(
            const android_studio::AndroidStudioEvent& asEvent,
            wireless_android_play_playlog::LogEvent* logEvent) = 0;

    const std::string& sessionId() const { return mSessionId; }

protected:
    MetricsWriter(const std::string& sessionId);

private:
    std::string mSessionId;
};

}  // namespace metrics
}  // namespace android

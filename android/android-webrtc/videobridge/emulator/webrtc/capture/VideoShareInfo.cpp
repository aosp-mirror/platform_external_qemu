// Copyright (C) 2018 The Android Open Source Project
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
#include "VideoShareInfo.h"

#include <fcntl.h>
#include <rtc_base/logging.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cstdint>

#include "android/base/memory/SharedMemory.h"

using android::base::SharedMemory;

namespace emulator {
namespace webrtc {
namespace videocapturemodule {

int32_t VideoShareInfo::GetDeviceName(uint32_t deviceNumber,
                                      char* deviceNameUTF8,
                                      uint32_t deviceNameLength,
                                      char* deviceUniqueIdUTF8,
                                      uint32_t deviceUniqueIdUTF8Length,
                                      char* productUniqueIdUTF8,
                                      uint32_t productUniqueIdUTF8Length) {
    memset(deviceNameUTF8, 0, deviceNameLength);
    memset(deviceUniqueIdUTF8, 0, deviceUniqueIdUTF8Length);
    snprintf(deviceNameUTF8, deviceNameLength, "%s", mHandle.c_str());
    snprintf(deviceUniqueIdUTF8, deviceUniqueIdUTF8Length, "%s",
             mHandle.c_str());
    return 0;
}

int32_t VideoShareInfo::Init() {
    SharedMemory shm(mHandle, sizeof(VideoInfo));
    if (!shm.isOpen()) {
        int err = shm.open(SharedMemory::AccessMode::READ_ONLY);
        if (err != 0) {
            RTC_LOG(LERROR) << "Unable to open memory mapped handle: ["
                            << mHandle << "] due to " << err;
            return -1;
        }
    }

    memcpy(&mCapability, *shm, sizeof(VideoInfo));
    // Note, we deliver kABGR, but that is not a format that is
    // supported!
    mCapability.videoType = ::webrtc::VideoType::kARGB;
    RTC_LOG(INFO) << "Discovered " << mCapability.width << "x"
                  << mCapability.height << " at: " << mCapability.maxFPS
                  << " fps";
    return 0;
}
}  // namespace videocapturemodule
}  // namespace webrtc
}  // namespace emulator

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
// GifConverter.h
//
// Class to convert a .webm/.mp4 file to GIF format.
//

#pragma once

#include "android/base/StringView.h"

#include <stdint.h>

namespace android {
namespace recording {

class GifConverter {
public:
    // Convert an mp4 or webm video into animated gif file.
    // params:
    //   inFilename - the input video file in webm or mp4 format
    //   outFilename - the output animated gif file
    //   bitRate - bit rate (bps) for the gif file, usually smaller number
    //             to reduce the file size.
    // returns:
    //   true if successfully converted to animated gif, false otherwise.
    static bool toAnimatedGif(android::base::StringView inFilename,
                              android::base::StringView outFilename,
                              uint32_t bitRate);
};

}  // namespace recording
}  // namespace android

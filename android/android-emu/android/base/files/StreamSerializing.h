// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/containers/SmallVector.h"
#include "android/base/files/Stream.h"

namespace android {
namespace base {

    // Snapshot a buffer
    template <class T>
    void onSaveBuffer(android::base::Stream* stream,
    		          const SmallVector<T>& buffer) {
        stream->putBe32(buffer.size());
        stream->write(buffer.data(),
                sizeof(T) * buffer.size());
    }
    // Load a buffer from snapshot
    template <class T>
    bool onLoadBuffer(android::base::Stream* stream,
    	              SmallVector<T>& buffer) {
        int32_t len = stream->getBe32();
        buffer.resize_noinit(len);
        int ret = (int)stream->read(buffer.begin(),
                                    len * sizeof(T));
        return ret == len * sizeof(T);
    }
}
}
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
// AVScopedPtr.h
//
// Helper to create smart pointers with custom deleters for various AV structs.
//
#pragma once

#include "android/base/memory/ScopedPtr.h"

#include <cassert>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

namespace android {
namespace recording {

// template specialization deleter helpers for av types
template <class T>
struct AVDeleter : std::false_type {
    static void deleteFunc(void* v) { assert(0); }
};

template <>  // explicit specialization for T = AVFrame
struct AVDeleter<AVFrame> : std::true_type {
    static void deleteFunc(void* f) {
        auto fp = static_cast<AVFrame*>(f);
        av_frame_free(&fp);
    }
};

template <>  // explicit specialization for T = SwsContext
struct AVDeleter<SwsContext> : std::true_type {
    static void deleteFunc(void* s) {
        sws_freeContext(static_cast<SwsContext*>(s));
    }
};

template <>  // explicit specialization for T = SwrContext
struct AVDeleter<SwrContext> : std::true_type {
    static void deleteFunc(void* s) {
        auto sp = static_cast<SwrContext*>(s);
        swr_free(&sp);
    }
};

template <>  // explicit specialization for T = AVPacket
struct AVDeleter<AVPacket> : std::true_type {
    static void deleteFunc(void* pkt) {
        av_packet_unref(static_cast<AVPacket*>(pkt));
    };
};

// Initialized from avcodec_alloc_context3()
template <>  // explicit specialization for T = AVCodecContext
struct AVDeleter<AVCodecContext> : std::true_type {
    static void deleteFunc(void* ctx) {
        auto ptr = static_cast<AVCodecContext*>(ctx);
        avcodec_free_context(&ptr);
    };
};

template <>  // explicit specialization for T = AVFormatContext
struct AVDeleter<AVFormatContext> : std::true_type {
    static void deleteFunc(void* oc) {
        auto ocp = static_cast<AVFormatContext*>(oc);
        if (ocp->iformat) {
            // input context
            avformat_close_input(&ocp);
        } else {
            // output context
            if (ocp->pb) {
                avio_closep(&ocp->pb);
            }
        }
        avformat_free_context(ocp);
    }
};

template <class T>
using AVScopedPtr =
        android::base::ScopedCustomPtr<T, decltype(&AVDeleter<T>::deleteFunc)>;

// A factory function that creates a scoped av pointer with the corresponding
// template specialized deleter.
template <class T>
AVScopedPtr<typename std::decay<typename std::remove_pointer<T>::type>::type>
makeAVScopedPtr(T data) {
    return android::base::makeCustomScopedPtr(
            data,
            AVDeleter<typename std::decay<
                    typename std::remove_pointer<T>::type>::type>::deleteFunc);
}

}  // namespace recording
}  // namespace android

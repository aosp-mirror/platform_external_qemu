// Copyright 2022 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

#if LIBAVCODEC_VERSION_MAJOR >= 58
#if !defined(CODEC_CAP_VARIABLE_FRAME_SIZE)
#define CODEC_CAP_VARIABLE_FRAME_SIZE (1 << 16)
#endif
#if !defined(CODEC_CAP_EXPERIMENTAL)
#define CODEC_CAP_EXPERIMENTAL (1 << 9)
#endif
#endif

#if LIBAVFORMAT_VERSION_MAJOR >= 58
#ifndef AVFMT_RAWPICTUR
#define AVFMT_RAWPICTURE 0x0020
#endif
#endif

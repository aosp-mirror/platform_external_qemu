/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (C) 2019  OpenSynergy GmbH
 *
 * This header is BSD licensed so anyone can use the definitions to
 * implement compatible drivers/servers.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of OpenSynergy GmbH nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL IBM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef VIRTIO_SND_IF_H
#define VIRTIO_SND_IF_H

#include <linux/types.h> /* __u{8,16,32,64} definitions */

#include "standard-headers/linux/virtio_ids.h"
#include "standard-headers/linux/virtio_types.h"

/*******************************************************************************
 * FEATURE BITS
 */
enum {
	/* output PCM stream support */
	VIRTIO_SND_F_PCM_OUTPUT = 0,
	/* input PCM stream support */
	VIRTIO_SND_F_PCM_INPUT
};

/*******************************************************************************
 * CONFIGURATION SPACE
 */

/* supported PCM sample formats */
enum {
	VIRTIO_SND_PCM_FMT_S8 = 0,
	VIRTIO_SND_PCM_FMT_U8,
	VIRTIO_SND_PCM_FMT_S16,
	VIRTIO_SND_PCM_FMT_U16,
	VIRTIO_SND_PCM_FMT_S32,
	VIRTIO_SND_PCM_FMT_U32,
	VIRTIO_SND_PCM_FMT_FLOAT,
	VIRTIO_SND_PCM_FMT_FLOAT64
};

#define VIRTIO_SND_PCM_FMTBIT(bit) \
	(1U << VIRTIO_SND_PCM_FMT_ ## bit)

enum {
	VIRTIO_SND_PCM_FMTBIT_S8 = VIRTIO_SND_PCM_FMTBIT(S8),
	VIRTIO_SND_PCM_FMTBIT_U8 = VIRTIO_SND_PCM_FMTBIT(U8),
	VIRTIO_SND_PCM_FMTBIT_S16 = VIRTIO_SND_PCM_FMTBIT(S16),
	VIRTIO_SND_PCM_FMTBIT_U16 = VIRTIO_SND_PCM_FMTBIT(U16),
	VIRTIO_SND_PCM_FMTBIT_S32 = VIRTIO_SND_PCM_FMTBIT(S32),
	VIRTIO_SND_PCM_FMTBIT_U32 = VIRTIO_SND_PCM_FMTBIT(U32),
	VIRTIO_SND_PCM_FMTBIT_FLOAT = VIRTIO_SND_PCM_FMTBIT(FLOAT),
	VIRTIO_SND_PCM_FMTBIT_FLOAT64 = VIRTIO_SND_PCM_FMTBIT(FLOAT64)
};

/* supported PCM frame rates */
enum {
	VIRTIO_SND_PCM_RATE_8000 = 0,
	VIRTIO_SND_PCM_RATE_11025,
	VIRTIO_SND_PCM_RATE_16000,
	VIRTIO_SND_PCM_RATE_22050,
	VIRTIO_SND_PCM_RATE_32000,
	VIRTIO_SND_PCM_RATE_44100,
	VIRTIO_SND_PCM_RATE_48000,
	VIRTIO_SND_PCM_RATE_64000,
	VIRTIO_SND_PCM_RATE_88200,
	VIRTIO_SND_PCM_RATE_96000,
	VIRTIO_SND_PCM_RATE_176400,
	VIRTIO_SND_PCM_RATE_192000
};

#define VIRTIO_SND_PCM_RATEBIT(bit) \
	(1U << VIRTIO_SND_PCM_RATE_ ## bit)

enum {
	VIRTIO_SND_PCM_RATEBIT_8000 = VIRTIO_SND_PCM_RATEBIT(8000),
	VIRTIO_SND_PCM_RATEBIT_11025 = VIRTIO_SND_PCM_RATEBIT(11025),
	VIRTIO_SND_PCM_RATEBIT_16000 = VIRTIO_SND_PCM_RATEBIT(16000),
	VIRTIO_SND_PCM_RATEBIT_22050 = VIRTIO_SND_PCM_RATEBIT(22050),
	VIRTIO_SND_PCM_RATEBIT_32000 = VIRTIO_SND_PCM_RATEBIT(32000),
	VIRTIO_SND_PCM_RATEBIT_44100 = VIRTIO_SND_PCM_RATEBIT(44100),
	VIRTIO_SND_PCM_RATEBIT_48000 = VIRTIO_SND_PCM_RATEBIT(48000),
	VIRTIO_SND_PCM_RATEBIT_64000 = VIRTIO_SND_PCM_RATEBIT(64000),
	VIRTIO_SND_PCM_RATEBIT_88200 = VIRTIO_SND_PCM_RATEBIT(88200),
	VIRTIO_SND_PCM_RATEBIT_96000 = VIRTIO_SND_PCM_RATEBIT(96000),
	VIRTIO_SND_PCM_RATEBIT_176400 = VIRTIO_SND_PCM_RATEBIT(176400),
	VIRTIO_SND_PCM_RATEBIT_192000 = VIRTIO_SND_PCM_RATEBIT(192000)
};

/* a PCM stream configuration */
struct virtio_pcm_stream_config {
	/* minimum # of supported channels */
	__u8 channels_min;
	/* maximum # of supported channels */
	__u8 channels_max;
	/* supported sample format bit map (VIRTIO_SND_PCM_FMTBIT_XXX) */
	__virtio16 formats;
	/* supported frame rate bit map (VIRTIO_SND_PCM_RATEBIT_XXX) */
	__virtio16 rates;

	__u16 padding;
};

/* a device configuration space */
struct virtio_snd_config {
	struct virtio_pcm_config {
		struct virtio_pcm_stream_config output;
		struct virtio_pcm_stream_config input;
	} pcm;
};

/*******************************************************************************
 * CONTROL REQUESTS
 */

enum {
	/* PCM control request types */
	VIRTIO_SND_R_PCM_CHMAP_INFO = 0,
	VIRTIO_SND_R_PCM_SET_FORMAT,
	VIRTIO_SND_R_PCM_PREPARE,
	VIRTIO_SND_R_PCM_START,
	VIRTIO_SND_R_PCM_STOP,
	VIRTIO_SND_R_PCM_PAUSE,
	VIRTIO_SND_R_PCM_UNPAUSE,

	/* generic status codes */
	VIRTIO_SND_S_OK = 0x8000,
	VIRTIO_SND_S_BAD_MSG,
	VIRTIO_SND_S_NOT_SUPP,
	VIRTIO_SND_S_IO_ERR
};

/* a generic control request/response header */
struct virtio_snd_hdr {
	union {
		/* VIRTIO_SND_R_XXX */
		__virtio32 code;
		/* VIRTIO_SND_S_XXX */
		__virtio32 status;
	};
};

/* supported PCM stream types */
enum {
	VIRTIO_SND_PCM_T_OUTPUT = 0,
	VIRTIO_SND_PCM_T_INPUT
};

/* PCM control request header */
struct virtio_snd_pcm_hdr {
	/* VIRTIO_SND_R_PCM_XXX */
	__virtio32 code;
	/* a PCM stream type (VIRTIO_SND_PCM_T_XXX) */
	__virtio32 stream;
};

/* set PCM stream format */
struct virtio_snd_pcm_set_format {
	/* VIRTIO_SND_R_PCM_SET_FORMAT */
	struct virtio_snd_pcm_hdr hdr;
	/* # of channels */
	__virtio16 channels;
	/* a PCM sample format (VIRTIO_SND_PCM_FMT_XXX) */
	__virtio16 format;
	/* a PCM frame rate (VIRTIO_SND_PCM_RATE_XXX) */
	__virtio16 rate;

	__u16 padding;
};

/* standard channel position definition */
enum {
	VIRTIO_SND_PCM_CH_NONE = 0,	/* undefined */
	VIRTIO_SND_PCM_CH_NA,		/* silent */
	VIRTIO_SND_PCM_CH_MONO,		/* mono stream */
	VIRTIO_SND_PCM_CH_FL,		/* front left */
	VIRTIO_SND_PCM_CH_FR,		/* front right */
	VIRTIO_SND_PCM_CH_RL,		/* rear left */
	VIRTIO_SND_PCM_CH_RR,		/* rear right */
	VIRTIO_SND_PCM_CH_FC,		/* front center */
	VIRTIO_SND_PCM_CH_LFE,		/* low frequency (LFE) */
	VIRTIO_SND_PCM_CH_SL,		/* side left */
	VIRTIO_SND_PCM_CH_SR,		/* side right */
	VIRTIO_SND_PCM_CH_RC,		/* rear center */
	VIRTIO_SND_PCM_CH_FLC,		/* front left center */
	VIRTIO_SND_PCM_CH_FRC,		/* front right center */
	VIRTIO_SND_PCM_CH_RLC,		/* rear left center */
	VIRTIO_SND_PCM_CH_RRC,		/* rear right center */
	VIRTIO_SND_PCM_CH_FLW,		/* front left wide */
	VIRTIO_SND_PCM_CH_FRW,		/* front right wide */
	VIRTIO_SND_PCM_CH_FLH,		/* front left high */
	VIRTIO_SND_PCM_CH_FCH,		/* front center high */
	VIRTIO_SND_PCM_CH_FRH,		/* front right high */
	VIRTIO_SND_PCM_CH_TC,		/* top center */
	VIRTIO_SND_PCM_CH_TFL,		/* top front left */
	VIRTIO_SND_PCM_CH_TFR,		/* top front right */
	VIRTIO_SND_PCM_CH_TFC,		/* top front center */
	VIRTIO_SND_PCM_CH_TRL,		/* top rear left */
	VIRTIO_SND_PCM_CH_TRR,		/* top rear right */
	VIRTIO_SND_PCM_CH_TRC,		/* top rear center */
	VIRTIO_SND_PCM_CH_TFLC,		/* top front left center */
	VIRTIO_SND_PCM_CH_TFRC,		/* top front right center */
	VIRTIO_SND_PCM_CH_TSL,		/* top side left */
	VIRTIO_SND_PCM_CH_TSR,		/* top side right */
	VIRTIO_SND_PCM_CH_LLFE,		/* left LFE */
	VIRTIO_SND_PCM_CH_RLFE,		/* right LFE */
	VIRTIO_SND_PCM_CH_BC,		/* bottom center */
	VIRTIO_SND_PCM_CH_BLC,		/* bottom left center */
	VIRTIO_SND_PCM_CH_BRC		/* bottom right center */
};

/* a maximum possible number of channels */
#define VIRTIO_SND_PCM_CH_MAX		256

/* response containing a PCM channel map information */
struct virtio_snd_pcm_chmap_info {
	/* VIRTIO_SND_S_XXX */
	__virtio32 status;
	/* # of valid entries in the positions array */
	__virtio32 npositions;
	/* PCM channel positions */
	__u8 positions[VIRTIO_SND_PCM_CH_MAX];
};

/* PCM I/O request header */
struct virtio_snd_pcm_xfer {
	/* a PCM stream type (VIRTIO_SND_PCM_T_XXX) */
	__virtio32 stream;
};

/* PCM I/O request status */
struct virtio_snd_pcm_xfer_status {
	/* VIRTIO_SND_S_XXX */
	__virtio32 status;
	/* an actual read/written amount of bytes */
	__virtio32 actual_length;
};

#endif /* VIRTIO_SND_IF_H */

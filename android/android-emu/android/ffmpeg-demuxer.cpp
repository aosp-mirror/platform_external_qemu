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

#include "android/ffmpeg-demuxer.h"

#include "android/utils/debug.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/avassert.h"
#include "libavutil/channel_layout.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libavutil/timestamp.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

#include <stdio.h>
#include <stdbool.h>
#ifdef _WIN32
#include <windows.h>
#endif
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

#ifndef _MSC_VER
#include <sys/time.h>
#else

#include <time.h>
#include <sys/timeb.h>

static int gettimeofday(struct timeval* tp, void* tz)
{
    struct _timeb timebuffer;
    _ftime (&timebuffer);
    tp->tv_sec = (long) timebuffer.time;
    tp->tv_usec = timebuffer.millitm * 1000;

    return 0;
}

#endif

#define D(...) VERBOSE_PRINT(media_dec, __VA_ARGS__)

static int g_registered = 0;
   
typedef struct ffmpeg_decoder
{
	int media_type; // Audio or video
	AVCodecContext* video_codec_context;
	AVCodecContext* audio_codec_context;
    AVFormatContext* fmt_context;
	AVCodec* codec;
	AVFrame* frame; // decoded video frame
	AVFrame* audio_frame; // decoded audio frame
	struct SwsContext *img_convert_ctx;
	int prepared;

	AVPacket pkt; // Encoded packet

	enum AVPixelFormat pixel_format;
	int decoded_size;
	int decoded_size_max;
    FrameDecodedCallback cb;
} ffmpeg_decoder;

/**
 * Allocate memory initialized to zero.
 * @param size
 */

void* xzalloc(size_t size)
{
	void* mem;

	if (size < 1)
		size = 1;

	mem = calloc(1, size);

	if (mem == NULL)
	{
		perror("xzalloc");
		derror("xzalloc: failed to allocate memory of size: %d\n", (int) size);
	}

	return mem;
}

/* get time in milliseconds */
/* not used
static unsigned int get_mstime(void)
{
	struct timeval tp;
	
	gettimeofday(&tp, 0);
	return (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}
*/

// Get the next frame, and sends the frame thru the callback.
// Callback will receive NULL once there are no more frames.
void ffmpeg_decode_next_frame(ffmpeg_decoder* decoder)
{
    if (decoder == NULL)
        return;

    if (av_read_frame(decoder->fmt_context, &decoder->pkt) < 0) {
        // Error or end of file
        decoder->cb();
        return;
    }

    // Decode the packet
    int got_frame = 0;
    int ret = avcodec_decode_video2(decoder->video_codec_context,
                                    decoder->frame,
                                    &got_frame,
                                    &decoder->pkt);
    if (ret < 0) {
        D("Error decoding video frame (%s)", av_err2str(ret));
        return;
    }
    
    decoder->cb();
}

static int ffmpeg_open_codec_context(ffmpeg_decoder* decoder, enum AVMediaType type) {
    int ret, stream_index;
    AVStream* st;
    AVCodecContext* dec_ctx = NULL;
    AVCodec* dec = NULL;
    
    ret = av_find_best_stream(decoder->fmt_context, type, -1, -1, NULL, 0);
    if (ret < 0) {
        D("Count not find %s stream in input file",
                av_get_media_type_string(type));
        return ret;
    } else {
        stream_index = ret;
        st = decoder->fmt_context->streams[stream_index];
        
        // Find decoder for the stream
        dec_ctx = st->codec;
        dec = avcodec_find_decoder(dec_ctx->codec_id);
        if (!dec) {
            D("Failed to find %s codec",
                    av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }

        // Open the decoders
        if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
            D("Failed to open %s codec",
                    av_get_media_type_string(type));
            return ret;
        }
        
        if (type == AVMEDIA_TYPE_VIDEO) {
            decoder->video_codec_context =
                    decoder->fmt_context->streams[stream_index]->codec;
        } else {
            decoder->audio_codec_context =
                    decoder->fmt_context->streams[stream_index]->codec;
        }
    }
    return 0;
}

ffmpeg_decoder* ffmpeg_create_decoder(bool wantAudio,
                                      const char* filename,
                                      FrameDecodedCallback cb)
{
	ffmpeg_decoder *decoder = (ffmpeg_decoder *)malloc(sizeof(ffmpeg_decoder));
	AVCodecContext* codec_context;
	AVCodec* codec = NULL;
	AVFrame* frame;	
    AVFormatContext* fmt_context;
	
	if (!g_registered)
	{
		//avcodec_init();
		avcodec_register_all();	
		g_registered = 1;
	}
	
	memset(decoder, 0, sizeof(ffmpeg_decoder));
    
    // open media file, and allocate format context
    if (avformat_open_input(&fmt_context, filename, NULL, NULL)) {
        D("Could not open file %s", filename);
        return NULL;
    }
    // retrieve stream info
    if (avformat_find_stream_info(fmt_context, NULL) < 0) {
        D("Could not find stream information");
        return NULL;
    }

    // get video decoder context
    if (ffmpeg_open_codec_context(decoder, AVMEDIA_TYPE_VIDEO) < 0) {
        D("Unable to open video context");
        return NULL; 
    }

    // TODO: Open audio decoder context
    /*
    if (ffmpeg_open_codec_context(decoder, AVMEDIA_TYPE_AUDIO) >= 0) {

    }
    */

    // dump input information to stderr
    av_dump_format(fmt_context, 0, filename, 0);

	frame = av_frame_alloc();
    if (!frame) {
        D("Could not allocate frame");
        return NULL;
    }

    D("Video codec format=%s",
            decoder->video_codec_context->codec_id == AV_CODEC_ID_H264 ?
            "AV_CODEC_ID_H264" : "AV_CODEC_ID_VP8");
    /*
    D("Audio codec format=%s",
            av_get_media_type_string(decoder->audio_codec_context->codec_id));
    */

    decoder->fmt_context = fmt_context;
	decoder->codec = codec;
	decoder->frame = frame;
    decoder->cb = cb;

	av_init_packet(&decoder->pkt);
    decoder->pkt.data = NULL;
    decoder->pkt.size = 0;

	return decoder;
}

void ffmpeg_free_decoder(ffmpeg_decoder* decoder)
{
    if (decoder == NULL)
        return;

	av_free(decoder->frame);	
	avcodec_close(decoder->video_codec_context);	
	avcodec_close(decoder->audio_codec_context);	
    avformat_close_input(&decoder->fmt_context);
	av_free(decoder->video_codec_context);
	av_free(decoder->audio_codec_context);

	free(decoder);
}


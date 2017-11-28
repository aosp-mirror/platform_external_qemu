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

//
// Copyright (c) 2003 Fabrice Bellard
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "android/ffmpeg-muxer.h"

#include "android/base/synchronization/Lock.h"
#include "android/base/system/System.h"
#include "android/emulation/AudioCaptureEngine.h"
#include "android/ffmpeg-audio-capture.h"
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#define STREAM_PIX_FMT AV_PIX_FMT_YUV420P /* default pix_fmt */

#define SCALE_FLAGS SWS_BICUBIC

#define DEBUG_VIDEO 0
#define DEBUG_AUDIO 0

#if DEBUG_VIDEO
#include <stdio.h>
#define D_V(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#define D_V(...) (void)0
#endif

#if DEBUG_AUDIO
#include <stdio.h>
#define D_A(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#define D_A(...) (void)0
#endif

using namespace android::emulation;

// a wrapper around a single output AVStream
struct VideoOutputStream {
    AVStream* st;

    AVCodecID codec_id;

    // video width and height
    int width;
    int height;

    int bit_rate;
    int fps;

#if DEBUG_VIDEO
    int64_t frame_count;
    int64_t write_frame_count;
#endif

    // pts of the next frame that will be generated
    int64_t next_pts;

    AVFrame* frame;
    AVFrame* tmp_frame;

    struct SwsContext* sws_ctx;
};

typedef struct AudioOutputStream {
    AVStream* st;

    AVCodecID codec_id;

    int bit_rate;
    int sample_rate;

#if DEBUG_AUDIO
    int64_t frame_count;
    int64_t write_frame_count;
#endif

    // pts of the next frame that will be generated
    int64_t next_pts;

    int samples_count;

    AVFrame* frame;
    AVFrame* tmp_frame;

    uint8_t* audio_leftover;
    int audio_leftover_len;

    struct SwrContext* swr_ctx;
} AudioOutputStream;

typedef struct ffmpeg_recorder {
    char* path;
    FfmpegAudioCapturer* audio_capturer;
    AVFormatContext* oc;
    VideoOutputStream video_st;
    AudioOutputStream audio_st;
    // A single lock to protect writing audio and video frames to the video file
    mutable android::base::Lock out_pkt_lock;
    bool have_video;
    bool have_audio;
    bool started;
    uint64_t start_time;
    int fb_width;
    int fb_height;
} ffmpeg_recorder;

// FFMpeg defines a set of macros for the same purpose, but those don't
// compile in C++ mode.
// Let's define regular C++ functions for it.
static std::string avTs2Str(int64_t ts) {
    char res[AV_TS_MAX_STRING_SIZE] = {};
    return av_ts_make_string(res, ts);
}

static std::string avTs2TimeStr(int64_t ts, AVRational* tb) {
    char res[AV_TS_MAX_STRING_SIZE] = {};
    return av_ts_make_time_string(res, ts, tb);
}

static std::string avErr2Str(int errnum) {
    char res[AV_ERROR_MAX_STRING_SIZE] = {};
    return av_make_error_string(res, AV_ERROR_MAX_STRING_SIZE, errnum);
}

static void log_packet(const AVFormatContext* fmt_ctx, const AVPacket* pkt) {
    AVRational* time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

#if DEBUG_VIDEO || DEBUG_AUDIO
    fprintf(stderr,
#else
    VERBOSE_PRINT(
            capture,
#endif
            "pts:%s pts_time:%s dts:%s dts_time:%s duration:%s "
            "duration_time:%s "
            "stream_index:%d\n",
            avTs2Str(pkt->pts).c_str(),
            avTs2TimeStr(pkt->pts, time_base).c_str(),
            avTs2Str(pkt->dts).c_str(),
            avTs2TimeStr(pkt->dts, time_base).c_str(),
            avTs2Str(pkt->duration).c_str(),
            avTs2TimeStr(pkt->duration, time_base).c_str(), pkt->stream_index);
}

static int write_frame(ffmpeg_recorder* recorder,
                       AVFormatContext* fmt_ctx,
                       const AVRational* time_base,
                       AVStream* st,
                       AVPacket* pkt) {
    // rescale output packet timestamp values from codec to stream timebase
    // av_packet_rescale_ts(pkt, *time_base, st->time_base);

    pkt->dts = AV_NOPTS_VALUE;
    pkt->stream_index = st->index;

    // Write the compressed frame to the media file.
    log_packet(fmt_ctx, pkt);

    android::base::AutoLock lock(recorder->out_pkt_lock);
    int rc = av_interleaved_write_frame(fmt_ctx, pkt);

    return rc;
}

// audio output
static AVFrame* alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate,
                                  int nb_samples) {
    AVFrame* frame = av_frame_alloc();
    int ret;

    if (!frame) {
        derror("Error allocating an audio frame\n");
        return NULL;
    }

    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            derror("Error allocating an audio buffer\n");
            return NULL;
        }
    }

    return frame;
}

static int open_audio(AVFormatContext* oc,
                      AVCodec* codec,
                      AudioOutputStream* ost,
                      AVDictionary* opt_arg) {
    AVCodecContext* c;
    int nb_samples;
    int ret;
    AVDictionary* opts = NULL;

    c = ost->st->codec;

    // open it
    av_dict_copy(&opts, opt_arg, 0);

    ret = avcodec_open2(c, codec, &opts);
    av_dict_free(&opts);
    if (ret < 0) {
        derror("Could not open audio codec: %s\n", avErr2Str(ret).c_str());
        return -1;
    }

    if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = c->frame_size;

    ost->frame = alloc_audio_frame(c->sample_fmt, c->channel_layout,
                                   c->sample_rate, nb_samples);
    ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout,
                                       c->sample_rate, nb_samples);

    // create resampler context
    ost->swr_ctx = swr_alloc();
    if (!ost->swr_ctx) {
        derror("Could not allocate resampler context\n");
        return -1;
    }

    // set options
    av_opt_set_int(ost->swr_ctx, "in_channel_count", c->channels, 0);
    av_opt_set_int(ost->swr_ctx, "in_sample_rate", c->sample_rate, 0);
    av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int(ost->swr_ctx, "out_channel_count", c->channels, 0);
    av_opt_set_int(ost->swr_ctx, "out_sample_rate", c->sample_rate, 0);
    av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt", c->sample_fmt, 0);

    // initialize the resampling context
    if ((ret = swr_init(ost->swr_ctx)) < 0) {
        derror("Failed to initialize the resampling context\n");
        return -1;
    }

    return 0;
}

//
// encode one audio frame and send it to the recorder
// return 1 when encoding is finished, 0 otherwise
//
static int write_audio_frame(ffmpeg_recorder* recorder,
                             AVFormatContext* oc,
                             AudioOutputStream* ost,
                             AVFrame* frame) {
    int dst_nb_samples;
    int ret;
    AVCodecContext* c = ost->st->codec;

    if (frame) {
        // convert samples from native format to destination codec format, using
        // the resampler compute destination number of samples
        dst_nb_samples = av_rescale_rnd(
                swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
                c->sample_rate, c->sample_rate, AV_ROUND_UP);
        av_assert0(dst_nb_samples == frame->nb_samples);

        // when we pass a frame to the encoder, it may keep a reference to it
        // internally;
        // make sure we do not overwrite it here
        //
        ret = av_frame_make_writable(ost->frame);
        if (ret < 0)
            return ret;

        // convert to destination format
        ret = swr_convert(ost->swr_ctx, ost->frame->data, dst_nb_samples,
                          (const uint8_t**)frame->data, frame->nb_samples);
        if (ret < 0) {
            derror("Error while converting\n");
            return ret;
        }
        ost->frame->pts = frame->pts;
        frame = ost->frame;
        ost->samples_count += dst_nb_samples;
    }

    AVPacket pkt = {0};  // data and size must be 0;
    int got_packet;

    D_A("Encoding audio frame %d\n", ost->frame_count++);
    av_init_packet(&pkt);
    ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        derror("Error encoding audio frame: %s\n", avErr2Str(ret).c_str());
        return ret;
    }

    if (got_packet) {
        D_A("%sWriting audio frame %d\n",
            pkt.flags & AV_PKT_FLAG_KEY ? "(KEY) " : "",
            ost->write_frame_count++);
#if DEBUG_AUDIO
        log_packet(oc, &pkt);
#endif
        ret = write_frame(recorder, oc, &c->time_base, ost->st, &pkt);
        av_packet_unref(&pkt);
        if (ret < 0) {
            derror("Error while writing audio frame: %s\n",
                   avErr2Str(ret).c_str());
            return ret;
        }
    }

    return (frame || got_packet) ? 0 : 1;
}

// video output
static AVFrame* alloc_picture(enum AVPixelFormat pix_fmt,
                              int width,
                              int height) {
    AVFrame* picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture)
        return NULL;

    picture->format = pix_fmt;
    picture->width = width;
    picture->height = height;

    // allocate the buffers for the frame data
    ret = av_frame_get_buffer(picture, 32);
    if (ret < 0) {
        derror("Could not allocate frame data.\n");
        return NULL;
    }

    return picture;
}

static int open_video(AVFormatContext* oc,
                      AVCodec* codec,
                      VideoOutputStream* ost,
                      AVDictionary* opt_arg) {
    int ret;
    AVCodecContext* c = ost->st->codec;
    AVDictionary* opts = NULL;

    av_dict_copy(&opts, opt_arg, 0);

    // vp9
    if (ost->st->codec->codec_id == AV_CODEC_ID_VP9) {
        av_dict_set(&opts, "deadline", "realtime" /*"good"*/, 0);
        av_dict_set(&opts, "cpu-used", "8", 0);
    }

    // open the codec
    ret = avcodec_open2(c, codec, &opts);
    av_dict_free(&opts);
    if (ret < 0) {
        derror("Could not open video codec: %s\n", avErr2Str(ret).c_str());
        return ret;
    }

    // allocate and init a re-usable frame
    ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
    if (!ost->frame) {
        derror("Could not allocate video frame\n");
        return -1;
    }

    // If the output format is not YUV420P, then a temporary YUV420P
    // picture is needed too. It is then converted to the required
    // output format.
    ost->tmp_frame = NULL;
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
        if (!ost->tmp_frame) {
            derror("Could not allocate temporary picture\n");
            return -1;
        }
    }

    return 0;
}

// encode one video frame and send it to the recorder
// return 1 when encoding is finished, 0 otherwise, negative if failed
static int write_video_frame(ffmpeg_recorder* recorder,
                             AVFormatContext* oc,
                             VideoOutputStream* ost,
                             AVFrame* frame) {
    int ret;
    AVCodecContext* c;
    int got_packet = 0;

    c = ost->st->codec;

    if (oc->oformat->flags & AVFMT_RAWPICTURE) {
        // a hack to avoid data copy with some raw video muxers
        AVPacket pkt;
        av_init_packet(&pkt);

        if (!frame)
            return 1;

        pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.stream_index = ost->st->index;
        pkt.data = (uint8_t*)frame;
        pkt.size = sizeof(AVPicture);

        pkt.pts = pkt.dts = frame->pts;
        av_packet_rescale_ts(&pkt, c->time_base, ost->st->time_base);

        android::base::AutoLock lock(recorder->out_pkt_lock);
        ret = av_interleaved_write_frame(oc, &pkt);
    } else {
        AVPacket pkt = {0};
        av_init_packet(&pkt);

        // encode the frame
        D_V("Encoding video frame %d\n", ost->frame_count++);
        ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
        if (ret < 0) {
            derror("Error encoding video frame: %s\n", avErr2Str(ret).c_str());
            return ret;
        }

        if (got_packet) {
            D_V("%sWriting frame %d\n",
                (pkt.flags & AV_PKT_FLAG_KEY) ? "(KEY) " : "",
                ost->write_frame_count++);
#if DEBUG_VIDEO
            log_packet(oc, &pkt);
#endif
            ret = write_frame(recorder, oc, &c->time_base, ost->st, &pkt);
            av_packet_unref(&pkt);
        } else {
            ret = 0;
        }
    }

    if (ret < 0) {
        derror("Error while writing video frame: %s\n", avErr2Str(ret).c_str());
        return ret;
    }

    return (frame || got_packet) ? 0 : 1;
}

static void close_audio_stream(AVFormatContext* oc, AudioOutputStream* ost) {
    avcodec_close(ost->st->codec);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    swr_free(&ost->swr_ctx);
}

static void close_video_stream(AVFormatContext* oc, VideoOutputStream* ost) {
    avcodec_close(ost->st->codec);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    sws_freeContext(ost->sws_ctx);
}

static bool has_suffix(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static bool sIsRegistered = false;

ffmpeg_recorder* ffmpeg_create_recorder(const RecordingInfo* info,
                                        int fb_width,
                                        int fb_height) {
    ffmpeg_recorder* recorder = NULL;
    AVFormatContext* oc = NULL;

    if (info == NULL || info->fileName == NULL)
        return NULL;

    recorder = (ffmpeg_recorder*)malloc(sizeof(ffmpeg_recorder));
    if (recorder == NULL) {
        return NULL;
    }

    memset(recorder, 0, sizeof(ffmpeg_recorder));
    recorder->path = strdup(info->fileName);

    // Initialize libavcodec, and register all codecs and formats. does not hurt
    // to register multiple times
    if (!sIsRegistered) {
        av_register_all();
        sIsRegistered = true;
    }

    // allocate the output media context
    avformat_alloc_output_context2(&oc, NULL, NULL, recorder->path);
    if (oc == NULL) {
        free(recorder);
        return NULL;
    }

    recorder->oc = oc;
    recorder->start_time = android::base::System::get()->getHighResTimeUs();

    if (has_suffix(recorder->path, ".webm")) {
        recorder->audio_st.codec_id = AV_CODEC_ID_VORBIS;
        recorder->video_st.codec_id = AV_CODEC_ID_VP9;
    } else {
        recorder->audio_st.codec_id = AV_CODEC_ID_AAC;
        recorder->video_st.codec_id = AV_CODEC_ID_H264;
    }

    recorder->fb_width = fb_width;
    recorder->fb_height = fb_height;

    return recorder;
}

void ffmpeg_delete_recorder(ffmpeg_recorder* recorder) {
    if (recorder == NULL)
        return;

    if (recorder->audio_capturer != NULL) {
        AudioCaptureEngine* engine = AudioCaptureEngine::get();
        if (engine != NULL)
            engine->stop(recorder->audio_capturer);
        delete recorder->audio_capturer;
    }

    // flush video encoding with a NULL frame
    while (recorder->have_video) {
        AVPacket pkt = {0};
        int got_packet = 0;
        av_init_packet(&pkt);
        int ret = avcodec_encode_video2(recorder->video_st.st->codec, &pkt,
                                        NULL, &got_packet);
        if (ret < 0 || !got_packet)
            break;
        D_V("%s: Writing frame %d\n", __func__,
            recorder->video_st.write_frame_count++);
        write_frame(recorder, recorder->oc,
                    &recorder->video_st.st->codec->time_base,
                    recorder->video_st.st, &pkt);
        av_packet_unref(&pkt);
    }

    // flush the remaining audio packet
    if (recorder->have_audio && (recorder->audio_st.audio_leftover_len > 0)) {
        AVFrame* frame = recorder->audio_st.tmp_frame;
        int frame_size = frame->nb_samples *
                         recorder->audio_st.st->codec->channels *
                         sizeof(int16_t);  // 16-bit
        if (recorder->audio_st.audio_leftover_len <
            frame_size) {  // this should always true
            int size = frame_size - recorder->audio_st.audio_leftover_len;
            uint8_t* zeros = new uint8_t[size];
            if (zeros != nullptr) {
                ffmpeg_encode_audio_frame(recorder, zeros, size);
                delete[] zeros;
            }
        }
    }

    // flush audio encoding with a NULL frame
    while (recorder->have_audio) {
        AVPacket pkt = {0};
        int got_packet;
        av_init_packet(&pkt);
        int ret = avcodec_encode_audio2(recorder->audio_st.st->codec, &pkt,
                                        NULL, &got_packet);
        if (ret < 0 || !got_packet)
            break;
        D_A("%s: Writing audio frame %d\n", __func__,
            recorder->audio_st.write_frame_count++);
        write_frame(recorder, recorder->oc,
                    &recorder->audio_st.st->codec->time_base,
                    recorder->audio_st.st, &pkt);
        av_packet_unref(&pkt);
    }

    // Write the trailer, if any. The trailer must be written before you
    // close the CodecContexts open when you wrote the header; otherwise
    // av_write_trailer() may try to use memory that was freed on
    // av_codec_close().
    av_write_trailer(recorder->oc);

    // Close each codec.
    if (recorder->have_video)
        close_video_stream(recorder->oc, &recorder->video_st);

    if (recorder->have_audio)
        close_audio_stream(recorder->oc, &recorder->audio_st);

    if (recorder->audio_st.audio_leftover != nullptr) {
        free(recorder->audio_st.audio_leftover);
    }

    // Close the output file.
    if (!(recorder->oc->oformat->flags & AVFMT_NOFILE))
        avio_closep(&recorder->oc->pb);

    // free the stream
    avformat_free_context(recorder->oc);

    if (recorder->path)
        free(recorder->path);

    free(recorder);
}

// local helper to start the recording, after tracks are added
static int start_recording(ffmpeg_recorder* recorder) {
    int ret;
    AVDictionary* opt = NULL;

    if (recorder == NULL)
        return -1;

    if (recorder->started)
        return 0;

    AVFormatContext* oc = recorder->oc;

    av_dump_format(oc, 0, recorder->path, 1);

    AVOutputFormat* fmt = oc->oformat;

    // open the output file, if needed
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, recorder->path, AVIO_FLAG_WRITE);
        if (ret < 0) {
            derror("Could not open '%s': %s\n", recorder->path,
                   avErr2Str(ret).c_str());
            free(recorder);
            return ret;
        }
    }

    // Write the stream header, if any.
    ret = avformat_write_header(oc, &opt);
    if (ret < 0) {
        derror("Error occurred when opening output file: %s\n",
               avErr2Str(ret).c_str());
        free(recorder);
        return ret;
    }

    VERBOSE_PRINT(capture, "recorder->audio_capturer=%p\n",
                  recorder->audio_capturer);
    if (recorder->audio_capturer != NULL) {
        AudioCaptureEngine* engine = AudioCaptureEngine::get();
        VERBOSE_PRINT(capture, "engine=%p\n", engine);
        if (engine != NULL)
            engine->start(recorder->audio_capturer);
    }

    recorder->started = true;
    return 0;
}

int ffmpeg_add_audio_track(ffmpeg_recorder* recorder,
                           int bit_rate,
                           int sample_rate) {
    if (recorder == NULL)
        return -1;

    AudioOutputStream* ost = &recorder->audio_st;
    AVCodecID codec_id = ost->codec_id;
    AVFormatContext* oc = recorder->oc;
    AVOutputFormat* fmt = oc->oformat;

    fmt->video_codec = codec_id;

    ost->bit_rate = bit_rate;
    ost->sample_rate = sample_rate;
#if DEBUG_AUDIO
    ost->frame_count = 0;
    ost->write_frame_count = 0;
#endif

    // find the encoder
    AVCodec* codec = avcodec_find_encoder(codec_id);
    if (codec == NULL) {
        derror("Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
        return -1;
    }

    ost->st = avformat_new_stream(oc, codec);
    if (!ost->st) {
        derror("Could not allocate stream\n");
        return -1;
    }

    ost->st->id = oc->nb_streams - 1;
    AVCodecContext* c = ost->st->codec;

    if (codec->type != AVMEDIA_TYPE_AUDIO) {
        return -1;
    }

    c->sample_fmt =
            codec->sample_fmts ? codec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
    c->bit_rate = bit_rate;
    c->sample_rate = sample_rate;

    int i;
    if (codec->supported_samplerates) {
        c->sample_rate = codec->supported_samplerates[0];
        for (i = 0; codec->supported_samplerates[i]; i++) {
            if (codec->supported_samplerates[i] == sample_rate)
                c->sample_rate = sample_rate;
        }
    }
    c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
    c->channel_layout = AV_CH_LAYOUT_STEREO;
    if (codec->channel_layouts) {
        c->channel_layout = codec->channel_layouts[0];
        for (i = 0; codec->channel_layouts[i]; i++) {
            if (codec->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                c->channel_layout = AV_CH_LAYOUT_STEREO;
        }
    }

    c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
    ost->st->time_base = (AVRational){1, 1000000 /*c->sample_rate*/};

    // c->time_base = ost->st->time_base;

    VERBOSE_PRINT(
            capture,
            "c->sample_fmt=%d, c->channels=%d, ost->st->time_base.den=%d\n",
            c->sample_fmt, c->channels, ost->st->time_base.den);

    // Some formats want stream headers to be separate.
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    recorder->have_audio = true;
    if (codec != NULL && (codec->capabilities & CODEC_CAP_EXPERIMENTAL) != 0) {
        ost->st->codec->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    }

    AVDictionary* opt = NULL;
    int ret = open_audio(oc, codec, &recorder->audio_st, opt);

    // start audio capture, this informs QEMU to send audio to our muxer
    if (ret == 0 && recorder->audio_capturer == NULL) {
        recorder->audio_capturer =
                new FfmpegAudioCapturer(recorder, sample_rate, 16, 2);
    }

    return ret;
}

int ffmpeg_add_video_track(ffmpeg_recorder* recorder,
                           int width,
                           int height,
                           int bit_rate,
                           int fps,
                           int intra_spacing) {
    if (recorder == NULL)
        return -1;

    VideoOutputStream* ost = &recorder->video_st;
    AVCodecID codec_id = ost->codec_id;
    AVFormatContext* oc = recorder->oc;
    AVOutputFormat* fmt = oc->oformat;

    fmt->video_codec = codec_id;

    ost->width = width;
    ost->height = height;
    ost->bit_rate = bit_rate;
    ost->fps = fps;
#if DEBUG_VIDEO
    ost->frame_count = 0;
    ost->write_frame_count = 0;
#endif

    // find the encoder
    AVCodec* codec = avcodec_find_encoder(codec_id);
    if (codec == NULL) {
        derror("Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
        return -1;
    }

    ost->st = avformat_new_stream(oc, codec);
    if (!ost->st) {
        derror("Could not allocate stream\n");
        return -1;
    }

    ost->st->id = oc->nb_streams - 1;
    AVCodecContext* c = ost->st->codec;

    if (codec->type != AVMEDIA_TYPE_VIDEO) {
        return -1;
    }

    c->codec_id = codec_id;

    c->bit_rate = ost->bit_rate;
    // Resolution must be a multiple of two.
    c->width = ost->width;
    c->height = ost->height;
    c->thread_count = 8;

    // timebase: This is the fundamental unit of time (in seconds) in terms
    // of which frame timestamps are represented. For fixed-fps content,
    // timebase should be 1/framerate and timestamp increments should be
    // identical to 1.
    if (codec_id == AV_CODEC_ID_VP9) {
        ost->st->time_base = (AVRational){1, fps};
    } else {
        ost->st->time_base = (AVRational){1, 1000000};  // microsecond timebase
    }
    c->time_base = ost->st->time_base;

    ost->st->avg_frame_rate = (AVRational){1, fps};

    c->gop_size =
            intra_spacing;  // emit one intra frame every twelve frames at most
    c->pix_fmt = STREAM_PIX_FMT;
    if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        // just for testing, we also add B frames
        c->max_b_frames = 2;
    }

    if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
        // Needed to avoid using macroblocks in which some coeffs overflow.
        // This does not happen with normal video, it just happens here as
        // the motion of the chroma plane does not match the luma plane.
        c->mb_decision = 2;
    }

    // Some formats want stream headers to be separate.
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    recorder->have_video = true;
    if (codec != NULL && (codec->capabilities & CODEC_CAP_EXPERIMENTAL) != 0) {
        ost->st->codec->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    }

    AVDictionary* opt = NULL;
    int ret = open_video(oc, codec, ost, opt);

    return ret;
}

// Encode and write a video frame (in 32-bit RGBA format) to the recoder
// params:
//    recorder - the recorder instance
//    rgb_pixels - the byte array for the pixel in RGBA format, each pixel take
//    4 byte
//    size - the rgb_pixels array size, it should be exactly as 4 * width *
//    height
// return:
//   0    if successful
//   < 0  if failed
//
// this method is thread safe
int ffmpeg_encode_video_frame(ffmpeg_recorder* recorder,
                              const uint8_t* rgb_pixels,
                              int size) {
    if (recorder == NULL)
        return -1;

    int rc = -1;

    if (!recorder->started) {
        rc = start_recording(recorder);
        if (rc < 0)
            return rc;
    }

    VideoOutputStream* ost = &recorder->video_st;
    AVCodecContext* c = ost->st->codec;

    if (ost->sws_ctx == NULL) {
        ost->sws_ctx = sws_getContext(
                recorder->fb_width, recorder->fb_height, AV_PIX_FMT_RGBA,
                c->width, c->height, c->pix_fmt, SCALE_FLAGS, NULL, NULL, NULL);
        if (ost->sws_ctx == NULL) {
            derror("Could not initialize the conversion context\n");
            return -1;
        }
    }

    const int linesize[1] = {4 * recorder->fb_width};
    sws_scale(ost->sws_ctx, (const uint8_t* const*)&rgb_pixels, linesize, 0,
              recorder->fb_height, ost->frame->data, ost->frame->linesize);

    uint64_t elapsedUS = android::base::System::get()->getHighResTimeUs() -
                         recorder->start_time;
    ost->frame->pts = (int64_t)(((double)elapsedUS * ost->st->time_base.den) /
                                1000000.00);
    rc = write_video_frame(recorder, recorder->oc, ost, ost->frame);

    return rc;
}

// Encode and write a video frame (in 32-bit RGBA format) to the recoder
// params:
//    recorder - the recorder instance
//    buffer - the byte array for the audio buffer in PCM format
//    size - the audio buffer size
// return:
//   0    if successful
//   < 0  if failed
//
// this method is thread safe
int ffmpeg_encode_audio_frame(ffmpeg_recorder* recorder,
                              uint8_t* buffer,
                              int size) {
    if (recorder == NULL)
        return -1;

    int rc = -1;

    if (!recorder->started) {
        rc = start_recording(recorder);
        if (rc < 0)
            return rc;
    }

    AudioOutputStream* ost = &recorder->audio_st;
    if (ost->st == NULL)
        return -1;

    if (recorder->oc == NULL)
        return -1;

    AVFrame* frame = ost->tmp_frame;
    int frame_size = frame->nb_samples * ost->st->codec->channels *
                     sizeof(int16_t);  // 16-bit

    // we need to split into frames
    int remaining = size;
    uint8_t* buf = buffer;

    if (ost->audio_leftover_len > 0) {
        if (ost->audio_leftover_len + size >= frame_size) {
            memcpy(ost->audio_leftover + ost->audio_leftover_len, buf,
                   frame_size - ost->audio_leftover_len);
            memcpy(frame->data[0], ost->audio_leftover, frame_size);
            uint64_t elapsedUS =
                    android::base::System::get()->getHighResTimeUs() -
                    recorder->start_time;
            int64_t pts = (int64_t)(
                    ((double)elapsedUS * ost->st->time_base.den) / 1000000.00);
            frame->pts = pts;
            write_audio_frame(recorder, recorder->oc, ost, frame);
            buf += (frame_size - ost->audio_leftover_len);
            remaining -= (frame_size - ost->audio_leftover_len);
            ost->audio_leftover_len = 0;
        } else {  // not enough for one frame yet
            memcpy(ost->audio_leftover + ost->audio_leftover_len, buf, size);
            ost->audio_leftover_len += size;
            remaining = 0;
        }
    }

    while (remaining >= frame_size) {
        memcpy(frame->data[0], buf, frame_size);
        uint64_t elapsedUS = android::base::System::get()->getHighResTimeUs() -
                             recorder->start_time;
        int64_t pts = (int64_t)(((double)elapsedUS * ost->st->time_base.den) /
                                1000000.00);
        frame->pts = pts;
        write_audio_frame(recorder, recorder->oc, ost, frame);
        buf += frame_size;
        remaining -= frame_size;
    }

    if (remaining > 0) {
        if (ost->audio_leftover == NULL)
            ost->audio_leftover = (uint8_t*)malloc(frame_size);

        memcpy(ost->audio_leftover, buf, remaining);
        ost->audio_leftover_len = remaining;
    }

    return 0;
}

// encode a frame, when *got_frame returns 1 means that
// the frame is fully encoded. If 0, means more data is
// required to input for encoding.
static int encode_write_frame(AVFormatContext* ofmt_ctx,
                              AVFrame* filt_frame,
                              unsigned int stream_index,
                              int* got_frame) {
    int ret;
    int got_frame_local;
    AVPacket enc_pkt;

    if (!got_frame)
        got_frame = &got_frame_local;

    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    av_init_packet(&enc_pkt);
    ret = avcodec_encode_video2(ofmt_ctx->streams[stream_index]->codec,
                                &enc_pkt, filt_frame, got_frame);
    if (ret < 0)
        return ret;
    if (!(*got_frame))
        return 0;

    // prepare packet for muxing
    enc_pkt.stream_index = stream_index;
    av_packet_rescale_ts(&enc_pkt,
                         ofmt_ctx->streams[stream_index]->codec->time_base,
                         ofmt_ctx->streams[stream_index]->time_base);
    // mux encoded frame
    ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);
    av_packet_unref(&enc_pkt);
    return ret;
}

static int flush_encoder(AVFormatContext* ofmt_ctx, unsigned int stream_index) {
    int ret;
    int got_frame;

    if (!(ofmt_ctx->streams[stream_index]->codec->codec->capabilities &
          AV_CODEC_CAP_DELAY))
        return 0;

    while (1) {
        av_log(NULL, AV_LOG_INFO, "Flushing stream #%u encoder\n",
               stream_index);
        ret = encode_write_frame(ofmt_ctx, NULL, stream_index, &got_frame);
        if (ret < 0)
            break;
        if (!got_frame)
            return 0;
    }
    return ret;
}

static void free_contxts(AVFormatContext* ifmt_ctx,
                         AVFormatContext* ofmt_ctx,
                         int stream_index) {
    avcodec_close(ifmt_ctx->streams[stream_index]->codec);
    if (ofmt_ctx->streams[stream_index] &&
        ofmt_ctx->streams[stream_index]->codec)
        avcodec_close(ofmt_ctx->streams[stream_index]->codec);

    avformat_close_input(&ifmt_ctx);
    if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);
}

// convert a mp4 or webm video into animated gif
// params:
//     input_video_file - the input video file in webm or mp4 format
//     output_video_file - the output animated gif file
//     gif_bit_rate - bit rate for the gif file, usually smaller number
//                    to reduce the file size
int ffmpeg_convert_to_animated_gif(const char* input_video_file,
                                   const char* output_video_file,
                                   int gif_bit_rate) {
    int ret;
    int video_stream_index = -1;

    // Initialize libavcodec, and register all codecs and formats. does not hurt
    // to register multiple times
    if (!sIsRegistered) {
        av_register_all();
        sIsRegistered = true;
    }

    // open the input video file
    AVFormatContext* ifmt_ctx = NULL;
    if ((ret = avformat_open_input(&ifmt_ctx, input_video_file, NULL, NULL)) <
        0) {
        derror("Cannot open input file:%s\n", input_video_file);
        return ret;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
        derror("Cannot find stream information\n");
        return ret;
    }

    av_dump_format(ifmt_ctx, 0, input_video_file, 0);

    // find the video stream, GIF supports only video, no audio
    for (int i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream* stream = ifmt_ctx->streams[i];
        AVCodecContext* codec_ctx = stream->codec;

        if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
            codec_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
            // Open decoder
            ret = avcodec_open2(
                    codec_ctx, avcodec_find_decoder(codec_ctx->codec_id), NULL);
            if (ret < 0) {
                derror("Failed to open decoder for stream #%u\n", i);
                return ret;
            }
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index == -1) {
        derror("Cannot find video stream\n");
        avformat_close_input(&ifmt_ctx);
        return -1;
    }

    // open the output gif file
    AVFormatContext* ofmt_ctx = NULL;
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, output_video_file);
    if (ofmt_ctx == NULL) {
        derror("Could not create output context for %s\n", output_video_file);
        avformat_close_input(&ifmt_ctx);
        return AVERROR_UNKNOWN;
    }

    AVStream* out_stream = avformat_new_stream(ofmt_ctx, NULL);
    if (out_stream == NULL) {
        derror("Failed allocating output stream\n");
        free_contxts(ifmt_ctx, ofmt_ctx, video_stream_index);
        return AVERROR_UNKNOWN;
    }

    AVStream* in_stream = ifmt_ctx->streams[video_stream_index];
    AVCodecContext* dec_ctx = in_stream->codec;
    AVCodecContext* enc_ctx = out_stream->codec;

    if (dec_ctx->codec_type != AVMEDIA_TYPE_VIDEO) {
        derror("Failed to find video stream\n");
        free_contxts(ifmt_ctx, ofmt_ctx, video_stream_index);
        return AVERROR_UNKNOWN;
    }

    AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_GIF);
    if (encoder == NULL) {
        derror("GIF encoder not found\n");
        free_contxts(ifmt_ctx, ofmt_ctx, video_stream_index);
        return AVERROR_INVALIDDATA;
    }

    if ((encoder->capabilities & CODEC_CAP_EXPERIMENTAL) != 0) {
        enc_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    }

    enc_ctx->height = dec_ctx->height;
    enc_ctx->width = dec_ctx->width;
    enc_ctx->bit_rate = gif_bit_rate;
    enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
    // take first format from list of supported formats, which should be
    // AV_PIX_FMT_RGB8
    enc_ctx->pix_fmt = encoder->pix_fmts[0];

    enc_ctx->time_base = dec_ctx->time_base;

    ret = avcodec_open2(enc_ctx, encoder, NULL);
    if (ret < 0) {
        derror("Cannot open video encoder for GIF\n");
        free_contxts(ifmt_ctx, ofmt_ctx, video_stream_index);
        return ret;
    }

    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    av_dump_format(ofmt_ctx, 0, output_video_file, 1);

    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, output_video_file, AVIO_FLAG_WRITE);
        if (ret < 0) {
            derror("Could not open output file '%s'", output_video_file);
            free_contxts(ifmt_ctx, ofmt_ctx, video_stream_index);
            return ret;
        }
    }

    // init muxer, write output file header
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        derror("Error occurred in avformat_write_header()\n");
        free_contxts(ifmt_ctx, ofmt_ctx, video_stream_index);
        return ret;
    }

    struct SwsContext* sws_ctx = NULL;
    if (dec_ctx->pix_fmt != enc_ctx->pix_fmt) {
        sws_ctx = sws_getContext(enc_ctx->width, enc_ctx->height,
                                 dec_ctx->pix_fmt, enc_ctx->width,
                                 enc_ctx->height,
                                 /*AV_PIX_FMT_RGBA*/ enc_ctx->pix_fmt,
                                 SCALE_FLAGS, NULL, NULL, NULL);
        if (sws_ctx == NULL) {
            derror("Could not initialize the conversion context\n");
            free_contxts(ifmt_ctx, ofmt_ctx, video_stream_index);
            return AVERROR(ENOMEM);
        }
    }

    // read all packets, decode, convert, then encode to gif format
    int64_t last_pts = -1;
    int got_frame;
    AVPacket packet = {0};
    packet.data = NULL;
    packet.size = 0;
    while (1) {
        if ((ret = av_read_frame(ifmt_ctx, &packet)) < 0)
            break;
        int stream_index = packet.stream_index;
        if (stream_index != video_stream_index)
            continue;

        // for GIF, it seems we can't use a shared frame, so we have to alloc
        // it here inside the loop
        AVFrame* frame = av_frame_alloc();
        if (!frame) {
            ret = AVERROR(ENOMEM);
            break;
        }

        AVFrame* tmp_frame = alloc_picture(enc_ctx->pix_fmt, enc_ctx->width,
                                           enc_ctx->height);
        if (!tmp_frame) {
            av_frame_free(&frame);
            ret = AVERROR(ENOMEM);
            break;
        }

        av_packet_rescale_ts(&packet,
                             ifmt_ctx->streams[stream_index]->time_base,
                             ifmt_ctx->streams[stream_index]->codec->time_base);
        ret = avcodec_decode_video2(ifmt_ctx->streams[stream_index]->codec,
                                    frame, &got_frame, &packet);
        if (ret < 0) {
            av_frame_free(&frame);
            av_frame_free(&tmp_frame);
            derror("Decoding failed\n");
            break;
        }

        ret = 0;
        if (got_frame) {
            frame->pts = av_frame_get_best_effort_timestamp(frame);
            // correct invalid pts in the rare cases
            if (frame->pts <= last_pts)
                frame->pts = last_pts + 1;
            last_pts = frame->pts;
            if (sws_ctx != NULL)
                sws_scale(sws_ctx, frame->data, frame->linesize, 0,
                          enc_ctx->height, tmp_frame->data,
                          tmp_frame->linesize);
            tmp_frame->pts = frame->pts;
            ret = encode_write_frame(ofmt_ctx, tmp_frame, stream_index, NULL);
            av_packet_unref(&packet);
            av_frame_free(&frame);
            av_frame_free(&tmp_frame);
            if (ret < 0) {
                break;
            }
        } else {
            av_frame_free(&frame);
            av_frame_free(&tmp_frame);
        }
    }

    if (ret == AVERROR_EOF)
        ret = 0;

    if (ret == 0) {
        // flush encoder
        flush_encoder(ofmt_ctx, video_stream_index);
        av_write_trailer(ofmt_ctx);
    }

    if (sws_ctx != NULL)
        sws_freeContext(sws_ctx);

    free_contxts(ifmt_ctx, ofmt_ctx, video_stream_index);
    return ret;
}

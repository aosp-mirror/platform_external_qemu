// Copyright (C) 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

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

#include "android/skin/qt/video-player/Decoder.h"
#include "android/skin/qt/video-player/VideoPlayer.h"
#include "android/utils/debug.h"

namespace android {
namespace videoplayer {

// ThreadWrapper implementations
intptr_t DecoderThread::main() {
    if (mType == ThreadType::AudioDecoding) {
        mPlayer->decodeAudioThread();
    } else if (mType == ThreadType::VideoDecoding) {
        mPlayer->decodeVideoThread();
    }

    return 0;
}

// Decoder implementations
Decoder::Decoder(VideoPlayer* player,
    AVCodecContext* avctx,
    PacketQueue* queue,
    VideoPlayerWaitInfo* empty_queue_cond) :
    mPlayer(player),
    mAvCtx(avctx),
    mPacketQueue(queue),
    mEmptyQueueCond(empty_queue_cond)
{
}

Decoder::~Decoder()
{
    av_packet_unref(&mPkt);
}

void Decoder::startThread(ThreadType type)
{
    mPacketQueue->start();

    mThread.reset(new DecoderThread(mPlayer, type));
    mThread->start();
}

void Decoder::abort()
{
    mPacketQueue->abort();
    if (mThread != nullptr) {
        mThread->wait();
        mThread = nullptr;
    }
    mPacketQueue->flush();
}

void Decoder::waitToFinish()
{
    if (mThread != nullptr) {
        mThread->wait();
        mThread = nullptr;
    }
}

int Decoder::decodeFrame(AVFrame *frame)
{
    int got_frame = 0;

    do {
        int ret = -1;
        if (mPacketQueue->isAbort()) {
            return -1;
        }

        if (!mPacketPending || mPacketQueue->getSerial() != mPktSerial) {
            AVPacket pkt;
            do {
                if (mPacketQueue->getNumPackets() == 0 && mEmptyQueueCond != nullptr) {
                    mEmptyQueueCond->lock.lock();
                    mEmptyQueueCond->done = true;
                    mEmptyQueueCond->cvDone.signalAndUnlock(&mEmptyQueueCond->lock);
                }
                if (mPacketQueue->get(&pkt, true, &mPktSerial) < 0) {
                    return -1;
                }
                if (pkt.data == PacketQueue::sFlushPkt.data) {
                    avcodec_flush_buffers(mAvCtx);
                    mFinished = false;
                    mNextPts = mStartPts;
                    mNextPtsTb = mStartPtsTb;
                }
            } while (pkt.data == PacketQueue::sFlushPkt.data || mPacketQueue->getSerial() != mPktSerial);
            av_packet_unref(&mPkt);
            mTmpPkt = mPkt = pkt;
            mPacketPending = true;
        }

        switch (mAvCtx->codec_type) {
            case AVMEDIA_TYPE_VIDEO:
                ret = avcodec_decode_video2(mAvCtx, frame, &got_frame, &mTmpPkt);
                if (got_frame) {
                    if (frame->pkt_pts != AV_NOPTS_VALUE) {
                        frame->pts = frame->pkt_pts;
                    } else {
                        frame->pts = av_frame_get_best_effort_timestamp(frame);
                    }
                }
                break;

            case AVMEDIA_TYPE_AUDIO:
                ret = avcodec_decode_audio4(mAvCtx, frame, &got_frame, &mTmpPkt);
                if (got_frame) {
                    AVRational tb = (AVRational){1, frame->sample_rate};
                    if (frame->pts != AV_NOPTS_VALUE) {
                        frame->pts = av_rescale_q(frame->pts, mAvCtx->time_base, tb);
                    } else if (frame->pkt_pts != AV_NOPTS_VALUE) {
                        frame->pts = av_rescale_q(frame->pkt_pts, av_codec_get_pkt_timebase(mAvCtx), tb);
                    } else if (mNextPts != AV_NOPTS_VALUE) {
                        frame->pts = av_rescale_q(mNextPts, mNextPtsTb, tb);
                    }
                    if (frame->pts != AV_NOPTS_VALUE) {
                        mNextPts = frame->pts + frame->nb_samples;
                        mNextPtsTb = tb;
                    }
                }
                break;

            default:
                derror("Codec type: %d is not yet handled", mAvCtx->codec_type);
                break;
        }

        if (ret < 0) {
            mPacketPending = false;
        } else {
            mTmpPkt.dts =
            mTmpPkt.pts = AV_NOPTS_VALUE;
            if (mTmpPkt.data != nullptr) {
                if (mAvCtx->codec_type != AVMEDIA_TYPE_AUDIO) {
                    ret = mTmpPkt.size;
                }
                mTmpPkt.data += ret;
                mTmpPkt.size -= ret;
                if (mTmpPkt.size <= 0) {
                    mPacketPending = false;
                }
            } else {
                if (!got_frame) {
                    mPacketPending = false;
                    mFinished = true;
                }
            }
        }
    } while (!got_frame && !mFinished);

    return got_frame;
}

} // namespace videoplayer
} // namespace android

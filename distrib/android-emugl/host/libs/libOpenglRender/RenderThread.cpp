/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "RenderThread.h"

#include "ErrorLog.h"
#include "FrameBuffer.h"
#include "ReadBuffer.h"
#include "RenderControl.h"
#include "RenderThreadInfo.h"
#include "TimeUtils.h"

#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "OpenGLESDispatch/GLESv1Dispatch.h"
#include "../../../shared/OpenglCodecCommon/ChecksumCalculatorThreadInfo.h"

#include "emugl/common/aemu_lock.h"

#include <unistd.h>

#define STREAM_BUFFER_SIZE 4*1024*1024

RenderThread::RenderThread(IOStream *stream, emugl::Mutex *lock,
        NetPipe_SharedMemState* msptr) :
        emugl::Thread(),
        m_lock(lock),
        m_stream(stream),
        m_shared(msptr) {}

RenderThread::~RenderThread() {
    delete m_stream;
}

// static
RenderThread* RenderThread::create(IOStream *stream, emugl::Mutex *lock, NetPipe_SharedMemState* msptr) {
    return new RenderThread(stream, lock, msptr);
}

void RenderThread::forceStop() {
    m_stream->forceStop();
}

#define NET_PIPE_BUF_SIZE 1024 * 1024 * 32

#ifndef _WIN32
int shared_mem_read(NetPipe_SharedMemState* sm, unsigned char** next_loc,
        uint32_t* next_start,
        uint32_t* next_end

        ) {
    bool wrapped = false;
    int need_to_read;
    unsigned char* res;

    // fprintf(stderr, "%s: waiting for lock\n", __FUNCTION__);
    pthread_mutex_lock(&sm->lock);

    if (sm->valid_start == sm->valid_end) {
        // in this case, we're waiting for data so
        // a. rewind valid_start/end
        // b. wait for next send
        sm->valid_start = 0;
        sm->valid_end = 0;
        pthread_cond_wait(&sm->cv_valid, &sm->lock);
    }
    // fprintf(stderr, "%s: begin [0x%lx 0x%lx]\n", __FUNCTION__, sm->valid_start, sm->valid_end);
    if (sm->valid_end < sm->valid_start) { wrapped = true; }
    if (wrapped) { fprintf(stderr, "%s: wrapped! WATCH OUT, THIS SHOULD NOT HAPPEN\n", __FUNCTION__); }
    if (wrapped) { need_to_read = NET_PIPE_BUF_SIZE - sm->valid_start + sm->valid_end; }
    else { need_to_read = sm->valid_end - sm->valid_start; }
    // fprintf(stderr, "%s; start=0x%lx need_to_read=%d\n", __FUNCTION__,  sm->valid_start, need_to_read);

    if (wrapped) { 
        res = (unsigned char*)malloc(need_to_read);
        memcpy(res, sm->buffer + sm->valid_start, NET_PIPE_BUF_SIZE - sm->valid_start);
        memcpy(res + NET_PIPE_BUF_SIZE - sm->valid_start, sm->buffer, sm->valid_end);
    } else {
        res = sm->buffer + sm->valid_start;
    }
    *next_loc = res;
    pthread_mutex_unlock(&sm->lock);
    *next_start = sm->valid_start;
    *next_end = sm->valid_end;
    return need_to_read;
}

void shared_mem_consume(NetPipe_SharedMemState* sm, int last) {
    // fprintf(stderr, "%s: consume %d\n", __FUNCTION__, last);
    bool wrapped = false;
    int wrap_frag_size;
    // fprintf(stderr, "%s: waiting for lock\n", __FUNCTION__);
    pthread_mutex_lock(&sm->lock);
    // fprintf(stderr, "%s: begin\n", __FUNCTION__);
    if (sm->valid_end < sm->valid_start) { fprintf(stderr, "%s: wrapped!\n", __FUNCTION__); wrapped = true; }
    if (wrapped) { 
        if (sm->valid_start + last > NET_PIPE_BUF_SIZE) {
            wrap_frag_size = sm->valid_start + last - NET_PIPE_BUF_SIZE;
            sm->valid_start = wrap_frag_size;
        } else {
            sm ->valid_start += last;
        }
    } else { sm->valid_start += last; }
    // fprintf(stderr, "%s: end\n", __FUNCTION__);
    int rt = pthread_mutex_unlock(&sm->lock);
    if (!rt) {

    // fprintf(stderr, "%s: exit\n", __FUNCTION__);
    } else {
    fprintf(stderr, "%s: err unlock mutex rv=%d errno=%d\n", __FUNCTION__, rt, rt);

    }
}
#else

int shared_mem_read(NetPipe_SharedMemState* sm, unsigned char** next_loc,
        uint32_t* next_start,
        uint32_t* next_end

        ) {
    bool wrapped = false;
    int need_to_read;
    unsigned char* res;

    // fprintf(stderr, "%s: waiting for lock\n", __FUNCTION__);
    aemu_mutex_lock(sm->lock);

    if (sm->valid_start == sm->valid_end) {
        // in this case, we're waiting for data so
        // a. rewind valid_start/end
        // b. wait for next send
        sm->valid_start = 0;
        sm->valid_end = 0;
        aemu_cond_wait(sm->cv_valid, sm->lock);
    }
    // fprintf(stderr, "%s: begin [0x%lx 0x%lx]\n", __FUNCTION__, sm->valid_start, sm->valid_end);
    if (sm->valid_end < sm->valid_start) { wrapped = true; }
    if (wrapped) { fprintf(stderr, "%s: wrapped! WATCH OUT, THIS SHOULD NOT HAPPEN\n", __FUNCTION__); }
    if (wrapped) { need_to_read = NET_PIPE_BUF_SIZE - sm->valid_start + sm->valid_end; }
    else { need_to_read = sm->valid_end - sm->valid_start; }
    // fprintf(stderr, "%s; start=0x%lx need_to_read=%d\n", __FUNCTION__,  sm->valid_start, need_to_read);

    if (wrapped) { 
        res = (unsigned char*)malloc(need_to_read);
        memcpy(res, sm->buffer + sm->valid_start, NET_PIPE_BUF_SIZE - sm->valid_start);
        memcpy(res + NET_PIPE_BUF_SIZE - sm->valid_start, sm->buffer, sm->valid_end);
    } else {
        res = sm->buffer + sm->valid_start;
    }
    *next_loc = res;
    aemu_mutex_unlock(sm->lock);
    *next_start = sm->valid_start;
    *next_end = sm->valid_end;
    return need_to_read;
}

void shared_mem_consume(NetPipe_SharedMemState* sm, int last) {
    // fprintf(stderr, "%s: consume %d\n", __FUNCTION__, last);
    bool wrapped = false;
    int wrap_frag_size;
    // fprintf(stderr, "%s: waiting for lock\n", __FUNCTION__);
    aemu_mutex_lock(sm->lock);
    // fprintf(stderr, "%s: begin\n", __FUNCTION__);
    if (sm->valid_end < sm->valid_start) { fprintf(stderr, "%s: wrapped!\n", __FUNCTION__); wrapped = true; }
    if (wrapped) { 
        if (sm->valid_start + last > NET_PIPE_BUF_SIZE) {
            wrap_frag_size = sm->valid_start + last - NET_PIPE_BUF_SIZE;
            sm->valid_start = wrap_frag_size;
        } else {
            sm ->valid_start += last;
        }
    } else { sm->valid_start += last; }
    // fprintf(stderr, "%s: end\n", __FUNCTION__);
    int rt = aemu_mutex_unlock(sm->lock);
    if (!rt) {

    // fprintf(stderr, "%s: exit\n", __FUNCTION__);
    } else {
    fprintf(stderr, "%s: err unlock mutex rv=%d errno=%d\n", __FUNCTION__, rt, rt);

    }
}
#endif


intptr_t RenderThread::main() {
    RenderThreadInfo tInfo;
    ChecksumCalculatorThreadInfo tChecksumInfo;

    //
    // initialize decoders
    //
    tInfo.m_glDec.initGL(gles1_dispatch_get_proc_func, NULL);
    tInfo.m_gl2Dec.initGL(gles2_dispatch_get_proc_func, NULL);
    initRenderControlContext(&tInfo.m_rcDec);

    ReadBuffer readBuf(STREAM_BUFFER_SIZE);

    int stats_totalBytes = 0;
    long long stats_t0 = GetCurrentTimeMS();

    //
    // open dump file if RENDER_DUMP_DIR is defined
    //
    const char *dump_dir = getenv("RENDERER_DUMP_DIR");
    FILE *dumpFP = NULL;
    if (dump_dir) {
        size_t bsize = strlen(dump_dir) + 32;
        char *fname = new char[bsize];
        snprintf(fname,bsize,"%s/stream_%p", dump_dir, this);
        dumpFP = fopen(fname, "wb");
        if (!dumpFP) {
            fprintf(stderr,"Warning: stream dump failed to open file %s\n",fname);
        }
        delete [] fname;
    }

        bool had_progress = false;
        uint32_t last_start = 0;
        uint32_t last_end = 0;
        unsigned char* next_read_loc;
    while (1) {

        // int stat = readBuf.getData(m_stream);
        // if (stat <= 0) {
        //     break;
        // }
        int stat = shared_mem_read(m_shared, &next_read_loc, &last_start, &last_end);
        // fprintf(stderr, "%s: read %d [0x%lx 0x%lx] data 0x%x 0x%x 0x%x\n", __FUNCTION__, stat, m_shared->valid_start, m_shared->valid_end,
        //         stat >= 4 ? *((unsigned int*)(next_read_loc)) : 0,
        //         stat >= 8 ? *((unsigned int*)(next_read_loc + 4)) : 0,
        //         stat >= 12 ? *((unsigned int*)(next_read_loc + 8)) : 0
        //         );
        // usleep(1000);
        if (stat < 0) { 
            fprintf(stderr, "%s: statbreak: stat=%d\n", __FUNCTION__, stat);
            continue; 
        }

        //
        // log received bandwidth statistics
        //
        // stats_totalBytes += readBuf.validData();
        stats_totalBytes += stat; // shared_mem_valid_data_size(m_shared);
        long long dt = GetCurrentTimeMS() - stats_t0;
        if (dt > 1000) {
            //float dts = (float)dt / 1000.0f;
            //printf("Used Bandwidth %5.3f MB/s\n", ((float)stats_totalBytes / dts) / (1024.0f*1024.0f));
            stats_totalBytes = 0;
            stats_t0 = GetCurrentTimeMS();
        }

        //
        // dump stream to file if needed
        //
        if (dumpFP) {
            // int skip = readBuf.validData() - stat;
            // int skip = shared_mem_valid_data_size(m_shared) - stat;
            // fwrite(readBuf.buf()+skip, 1, readBuf.validData()-skip, dumpFP);
            // fwrite(shared_mem_next_read_loc(m_shared) + skip, 1, shared_mem_valid_data_size(m_shared) - skip, dumpFP);
            // fflush(dumpFP);
        }

        bool progress;
        do {
            progress = false;


            // m_lock->lock();
            //
            // try to process some of the command buffer using the GLESv1 decoder
            //
            //fprintf(stderr, "%s: try gl1 with pos=0x%lx len=%d\n", __FUNCTION__, next_read_loc, stat);
            size_t last = tInfo.m_glDec.decode(next_read_loc, stat, m_stream);
            // fprintf(stderr, "%s: gl1 last=%d\n", __FUNCTION__, last);
                   //  
                   //                             shared_mem_valid_data_size(m_shared),
                   //                             m_stream);
            if (last > 0) {
                progress = true;
                had_progress = true;
                // stat -= last;
                shared_mem_consume(m_shared, last);
                next_read_loc += last;
                stat -= last;
                // stat = shared_mem_read(m_shared, &next_read_loc);
                // continue;
                // readBuf.consume(last);
            }

            //
            // try to process some of the command buffer using the GLESv2 decoder
            //
            //fprintf(stderr, "%s: try gl2 with pos=0x%lx len=%d\n", __FUNCTION__, next_read_loc, stat);
            last = tInfo.m_gl2Dec.decode(next_read_loc, stat, m_stream);
            // fprintf(stderr, "%s: gl2 last=%d\n", __FUNCTION__, last);
            if (last > 0) {
                progress = true;
                had_progress = true;
                // stat -= last;
                // readBuf.consume(last);
                shared_mem_consume(m_shared, last);
                next_read_loc += last;
                stat -= last;
                // stat = shared_mem_read(m_shared, &next_read_loc);
                // continue;
            }

            //
            // try to process some of the command buffer using the
            // renderControl decoder
            //
            //fprintf(stderr, "%s: try rc with pos=0x%lx len=%d\n", __FUNCTION__, next_read_loc, stat);
            last = tInfo.m_rcDec.decode(next_read_loc, stat, m_stream);
            // fprintf(stderr, "%s: rc last=%d\n", __FUNCTION__, last);
            if (last > 0) {
                progress = true;
                had_progress = true;
                // readBuf.consume(last);
                shared_mem_consume(m_shared, last);
                next_read_loc += last;
                stat -= last;
                // stat = shared_mem_read(m_shared, &next_read_loc);
                //stat -= last;
                // continue;
            }

            // m_lock->unlock();

            //fprintf(stderr, "%s: progress: %d stat=%d\n", __FUNCTION__, progress, stat);
        } while( progress );

        // pthread_mutex_lock(&m_shared->lock);
        if (stat > 0) {
        // fprintf(stderr, "%s: warning: has remaining %d. this breaks assumtions about fully reading the stream. need 2 wait for more data (partial read).\n", __FUNCTION__, stat);
        // pthread_cond_wait(&m_shared->cv_valid, &m_shared->lock);
        }
        // pthread_mutex_unlock(&m_shared->lock);
        // sleep a bit
        // usleep(100);

    }

    if (dumpFP) {
        fclose(dumpFP);
    }

    //
    // Release references to the current thread's context/surfaces if any
    //
    FrameBuffer::getFB()->bindContext(0, 0, 0);
    if (tInfo.currContext || tInfo.currDrawSurf || tInfo.currReadSurf) {
        fprintf(stderr, "ERROR: RenderThread exiting with current context/surfaces\n");
    }

    FrameBuffer::getFB()->drainWindowSurface();

    FrameBuffer::getFB()->drainRenderContext();

    DBG("Exited a RenderThread\n");

    return 0;
}

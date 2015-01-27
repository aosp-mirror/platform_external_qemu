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
#ifndef _LIB_OPENGL_RENDER_RENDER_THREAD_H
#define _LIB_OPENGL_RENDER_RENDER_THREAD_H

#include "IOStream.h"
#include "GLDecoder.h"
<<<<<<< HEAD   (defcbc Merge "Fix missing backspace key" automerge: 35c966c  -s our)

#include "emugl/common/mutex.h"
#include "emugl/common/thread.h"

// A class used to model a thread of the RenderServer. Each one of them
// handles a single guest client / protocol byte stream.
class RenderThread : public emugl::Thread {
public:
    // Create a new RenderThread instance.
    // |stream| is an input stream that will be read from the thread,
    // and deleted by it when it exits.
    // |mutex| is a pointer to a shared mutex used to serialize
    // decoding operations between all threads.
    // TODO(digit): Why is this needed here? Shouldn't this be handled
    //              by the decoders themselves or at a lower-level?
    static RenderThread* create(IOStream* stream, emugl::Mutex* mutex);

    // Destructor.
    virtual ~RenderThread();

    // Returns true iff the thread has finished.
    // Note that this also means that the thread's stack has been
    bool isFinished() { return tryWait(NULL); }

    // Force a thread to stop.
    void forceStop();

private:
    RenderThread();  // No default constructor

    RenderThread(IOStream* stream, emugl::Mutex* mutex);

    virtual intptr_t main();

    emugl::Mutex* m_lock;
    IOStream* m_stream;
=======
#include "renderControl_dec.h"

#include "emugl/common/mutex.h"
#include "emugl/common/thread.h"

class RenderThread : public emugl::Thread
{
public:
    static RenderThread* create(IOStream* p_stream, emugl::Mutex* mutex);
    virtual ~RenderThread();
    bool isFinished() const { return m_finished; }

private:
    RenderThread(IOStream* p_stream, emugl::Mutex* mutex);
    virtual intptr_t main();

private:
    emugl::Mutex *m_lock;
    IOStream *m_stream;
    renderControl_decoder_context_t m_rcDec;
    bool m_finished;
>>>>>>> BRANCH (1556aa Merge changes I8781cc8c,If2010577)
};

#endif

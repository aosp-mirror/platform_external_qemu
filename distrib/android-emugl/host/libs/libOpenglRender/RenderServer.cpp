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
#include "RenderServer.h"

#include "RenderThread.h"
#include "TcpStream.h"
#ifndef _WIN32
#include "UnixStream.h"
#include <signal.h>
#include <pthread.h>
#endif
#ifdef _WIN32
#include "Win32PipeStream.h"
#endif

#include "OpenglRender/render_api.h"

#include <algorithm>
#include <memory>
#include <vector>

#include <string.h>

using RenderThreadPtr = std::unique_ptr<RenderThread>;
using RenderThreadsSet = std::vector<RenderThreadPtr>;

RenderServer::RenderServer() : m_lock(), m_listenSock(nullptr), m_exiting(false) {}

RenderServer::~RenderServer() {
    delete m_listenSock;
}

extern "C" int gRendererStreamMode;

RenderServer* RenderServer::create(char* addr, size_t addrLen) {
    auto server = std::unique_ptr<RenderServer>(new RenderServer());
    if (!server) {
        return nullptr;
    }

    if (gRendererStreamMode == RENDER_API_STREAM_MODE_TCP) {
        server->m_listenSock = new TcpStream();
    } else {
#ifdef _WIN32
        server->m_listenSock = new Win32PipeStream();
#else
        server->m_listenSock = new UnixStream();
#endif
    }

    if (!server->m_listenSock) {
        ERR("RenderServer::create failed to create a listening socket\n");
        return nullptr;
    }

    char addrstr[SocketStream::MAX_ADDRSTR_LEN];
    if (server->m_listenSock->listen(addrstr) < 0) {
        ERR("RenderServer::create failed to listen\n");
        return nullptr;
    }

    size_t len = strlen(addrstr) + 1;
    if (len > addrLen) {
        ERR("RenderServer address name too big for provided buffer: %zu > "
            "%zu\n",
            len, addrLen);
        return nullptr;
    }
    memcpy(addr, addrstr, len);

    return server.release();
}

intptr_t RenderServer::main() {
    RenderThreadsSet threads;

#ifndef _WIN32
    sigset_t set;
    sigfillset(&set);
    pthread_sigmask(SIG_SETMASK, &set, nullptr);
#endif

    while (1) {
        std::unique_ptr<SocketStream> stream(m_listenSock->accept());
        if (!stream) {
            fprintf(stderr, "Error accepting connection, skipping\n");
            continue;
        }

        unsigned int clientFlags;
        if (!stream->readFully(&clientFlags, sizeof(clientFlags))) {
            fprintf(stderr, "Error reading clientFlags\n");
            continue;
        }

        DBG("RenderServer: Got new stream!\n");

        // check if we have been requested to exit while waiting on accept
        if ((clientFlags & IOSTREAM_CLIENT_EXIT_SERVER) != 0) {
            m_exiting = true;
            break;
        }

        RenderThreadPtr rt(RenderThread::create(stream.get(), &m_lock));
        if (!rt) {
            fprintf(stderr, "Failed to create RenderThread\n");
        } else {
            stream.release();
            if (!rt->start()) {
                fprintf(stderr, "Failed to start RenderThread\n");
                rt.reset();
            }
        }

        //
        // remove from the threads list threads which are
        // no longer running
        //
        threads.erase(std::remove_if(threads.begin(), threads.end(),
                                     [](const RenderThreadPtr& ptr) {
                                         return ptr->isFinished();
                                     }),
                      threads.end());

        // if the thread has been created and started, insert it to the list
        if (rt) {
            threads.emplace_back(std::move(rt));
            DBG("Started new RenderThread (total %d)\n", (int)threads.size());
        }
    }

    //
    // Wait for all threads to finish
    //
    for (const RenderThreadPtr& ptr : threads) {
        ptr->forceStop();
    }
    for (const RenderThreadPtr& ptr : threads) {
        ptr->wait(nullptr);
    }

    return 0;
}

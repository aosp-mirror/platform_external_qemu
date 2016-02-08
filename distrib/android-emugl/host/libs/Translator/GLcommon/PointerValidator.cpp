/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License")
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

#include "GLcommon/PointerValidator.h"
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#ifdef _WIN32
#include <fcntl.h>
#include <windows.h>
#endif

// Mac doesn't have this macro
#ifndef TEMP_FAILURE_RETRY
// Windows doesn't need it
#ifdef _WIN32
#define TEMP_FAILURE_RETRY(exp) (exp)

#else // !_WIN32
#define TEMP_FAILURE_RETRY(exp)            \
  ({                                       \
    ssize_t _rc;                           \
    do {                                   \
      _rc = (exp);                         \
    } while (_rc == -1 && errno == EINTR); \
    _rc;                                   \
  })

#endif // !_WIN32
#endif // TEMP_FAILURE_RETRY

PointerValidator::PointerValidator() {
    int pipeReturnCode = -1;

#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    m_pageSize = si.dwPageSize;
    // The pipe has buffer size 1, because we only write and read one byte each time
    pipeReturnCode = _pipe(m_pipeFd, 1, _O_BINARY);

#else // !_WIN32
    m_pageSize = getpagesize();
    pipeReturnCode = pipe(m_pipeFd);

#endif // !_WIN32

    if (pipeReturnCode < 0) {
        // create pipe failed
        m_pipeFd[0] = 0;
        m_pipeFd[1] = 0;
        fprintf(stderr, "%s: Warning: pointer validation not usable\n",
                __FUNCTION__);
    }
}

PointerValidator::~PointerValidator() {
    if (m_pipeFd[0]) {
        close(m_pipeFd[0]);
    }
    if (m_pipeFd[1]) {
        close(m_pipeFd[1]);
    }
}

bool PointerValidator::isValid(const void* ptr) const {
    // Can't use the pipe, skip the test
    if (m_pipeFd[0] == 0) {
        return true;
    }

    // write returns -1 when pointer is invalid
    if (TEMP_FAILURE_RETRY(write(m_pipeFd[1], ptr, 1)) > 0) {
        char readBuf;
        // read it back, to avoid filling up the pipe
        TEMP_FAILURE_RETRY(read(m_pipeFd[0], &readBuf, 1));
        return true;
    } else {
        return false;
    }
}

bool PointerValidator::isValid(const void* buffer, size_t size) const {
    if (size == 0) return true;

    // check one byte in each page
    for (size_t i = 0; i < size; i += m_pageSize) {
        if (!isValid((const char*)buffer + i)) {
            return false;
        }
    }

    // the last byte of the buffer
    const void* lastByte = (const char*)buffer + size - 1;
    // the last byte we checked
    const void* lastCheck = (const char*)buffer + (size - 1) / m_pageSize * m_pageSize;
    // if they aren't on the same page, we need to check the last byte of the buffer
    if ((size_t)lastByte / m_pageSize != (size_t)lastCheck / m_pageSize
            && !isValid(lastByte) ) {
        return false;
    }

    return true;
}

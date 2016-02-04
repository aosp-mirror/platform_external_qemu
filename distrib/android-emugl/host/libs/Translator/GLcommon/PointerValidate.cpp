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

#include <GLcommon/PointerValidate.h>
#include <unistd.h>
#include <stdio.h>

#ifdef __linux__

PointerValidate::PointerValidate() {
    m_pageSize = getpagesize();
    if (pipe(m_pipeFd) < 0) {
        // create pipe failed
        m_pipeFd[0] = 0;
        m_pipeFd[1] = 0;
        fprintf(stderr, "%s: Warning: pointer validation not usable\n",
                __FUNCTION__);
    }
}

PointerValidate::~PointerValidate() {
    if (m_pipeFd[0]) {
        close(m_pipeFd[0]);
    }
    if (m_pipeFd[1]) {
        close(m_pipeFd[1]);
    }
}

bool PointerValidate::isValid(const void* ptr) const {
    // Can't use the pipe, skip the test
    if (m_pipeFd[0] == 0) {
        return true;
    }

    // write returns -1 when pointer is invalid
    if (write(m_pipeFd[1], ptr, 1) > 0) {
        char readBuf;
        // read it back, to avoid filling up the pipe
        read(m_pipeFd[0], &readBuf, 1); 
        return true;
    } else {
        return false;
    }
}

bool PointerValidate::isValid(const void* buffer, unsigned int size) const {
    // check one byte in each page
    for (unsigned int i=0; i<size; i+=m_pageSize) {
        if (!isValid((const char*)buffer+i)) {
            return false;
        }
    }

    // check the last byte, in case it happens to cross the boundary of a page
    if (size>1 && !isValid((const char*)buffer+size-1)) {
        return false;
    }

    return true;
}

#else  // __linux__
// win and mac not yet implemented
PointerValidate::PointerValidate() {}

PointerValidate::~PointerValidate() {}

bool PointerValidate::isValid(const void* ptr) const {
    return true;
}

bool PointerValidate::isValid(const void* buffer, unsigned int size) const {
    return true;
}

#endif  // __linux__

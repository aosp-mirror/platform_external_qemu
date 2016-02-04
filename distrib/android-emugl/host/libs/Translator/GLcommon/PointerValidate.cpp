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
    if (pipe(m_Pipefd) < 0) {
        // create pipe failed
        m_Pipefd[0] = 0;
        m_Pipefd[1] = 0;
        fprintf(stderr, "%s: Warning: pointer validation not usable\n",
                __FUNCTION__);
    }
}

PointerValidate::~PointerValidate() {
    if (m_Pipefd[0]) {
        close(m_Pipefd[0]);
    }
    if (m_Pipefd[1]) {
        close(m_Pipefd[1]);
    }
}

bool PointerValidate::isValid(const void* ptr) {
    // Can't use the pipe, skip the test
    if (m_Pipefd[0] == 0) {
        return true;
    }

    // write returns -1 when pointer is invalid
    if (write(m_Pipefd[1], ptr, 1) > 0) {
        char readBuf;
        read(m_Pipefd[0], &readBuf,
             1);  // read it back, to avoid filling up the pipe
        return true;
    } else {
        return false;
    }
}

#else  // __linux__
// win and mac not yet implemented
PointerValidate::PointerValidate() {}

PointerValidate::~PointerValidate() {}

bool PointerValidate::isValid(const void* ptr) {
    return true;
}

#endif  // __linux__

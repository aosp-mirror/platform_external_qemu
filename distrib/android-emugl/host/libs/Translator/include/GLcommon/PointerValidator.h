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

#pragma once

#include <stddef.h>

// This class is used to check if the addresses calculated from gl commands
// (glVertexAttribPointer, glVertexPointer, etc) are located in user's address
// space. Not doing such check before calling glDrawArrays / glDrawElements can
// crash GL drivers.
// Notice that passing the address check does not mean that the address is valid
// and safe. It simply means you don't immediately crash with a SIGSEGV if you
// READ from it. And in fact, one can still pass the test if the address is
// invalid but it happens to be in one of his allocated memory pages.
//
// In short, its only valid usage is trying to avoid crash when reading from an
// address provided by users. Using it for other any other purposes is dangerous
// and highly discouraged.
// Also, This class is not thread-safe. It is supposed to be used in GLEScontext,
// which is not shared between different threads.

class PointerValidator {
public:
    PointerValidator();
    ~PointerValidator();
    // returns true if ptr is in allocated memory pages
    bool isValid(const void* ptr) const;
    // returns true if the whole buffer is in allocated memory pages
    // when size=0, always returns true
    bool isValid(const void* buffer, size_t size) const;
private:
#ifdef __linux__
    int m_pipeFd[2];
    int m_pageSize;
#else   // __linux__
#endif  // __linux__
};

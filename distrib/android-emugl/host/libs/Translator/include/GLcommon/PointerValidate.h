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

// This class is not thread-safe. It is supposed to be used in GLEScontext,
// which is not shared between different threads so it is OK.

class PointerValidate {
public:
    PointerValidate();
    ~PointerValidate();
    bool isValid(const void* ptr) const;
    bool isValid(const void* buffer, unsigned int size) const;
private:
#ifdef __linux__
    int m_pipeFd[2];
    int m_pageSize;
#else   // __linux__
#endif  // __linux__
};

// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef ANDROID_KERNEL_KERNEL_UTILS_TESTING_H
#define ANDROID_KERNEL_KERNEL_UTILS_TESTING_H

namespace android {

namespace base {
class String;
}  // namespace base

namespace kernel {

// Type of a function used to retrieve the textual description of a given
// file at |filePath|. On success, return true and sets |*text| to the
// description text, as if running through the 'file' command on Unix.
// |opaque| is a client-provided value set by calling
// setFileDescriptionFunction() below.
typedef bool (GetFileDescriptionFunction)(void* opaque,
                                          const char* filePath,
                                          android::base::String* text);

// Change the implementation of the function that extracts textual
// descriptions from a given file. |file_func| is a pointer to the
// new function, and |file_opaque| is the value that will be passed
// as its first parameter. Note that if |file_func| is NULL, the
// default implementation will be selected instead.
//
// Only use this during unit-testing to force different description
// values on arbitrary file content.
void setFileDescriptionFunction(GetFileDescriptionFunction* file_func,
                                void* file_opaque);

}  // namespace kernel

}  // namespace android

#endif  // ANDROID_KERNEL_KERNEL_UTILS_TESTING_H

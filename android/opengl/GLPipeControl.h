// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

#include "android/base/Compiler.h"
#include "android/base/synchronization/Lock.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace android {
namespace opengl {

// GLPipeControl class is meant to track all active
// OpenGL pipes (which correspond to rendering threads).
// The main purpose of this class is to be able to selectively
// remove active pipes without a guest signal. This
// is useful for -gpu host snapshotting and for cleaning up
// all OpenGL pipes when the emulator exits.
class GLPipeControl {
public:
    GLPipeControl() :
        mCallback(nullptr), mLock(), mPipes() { }

    static GLPipeControl* get();

    void registerRemoveCallback(void (*f)(void*));

    // addPipe/removePipe are used by OpenglEsPipe
    // to communicate current state of pipes.
    void addPipe(void*);
    void removePipe(void*);

    // stopPipe is the main user-facing function.
    // Given a handle (uint64_t) to an existing pipe,
    // stopPipe calls the remove callback |mCallback|.
    // The callback may end up removing the pipe from
    // the list of tracked pipes.
    void stopPipe(uint64_t);

    // stopAll is a convenience function to stop every
    // OpenGL pipe.
    void stopAll();

    // listPipes is a convenience function for obtaining
    // active pipes.
    std::vector<void*> listPipes();
private:
    void (*mCallback)(void*);
    android::base::Lock mLock;
    std::unordered_map<uint64_t, void*> mPipes;

    DISALLOW_COPY_ASSIGN_AND_MOVE(GLPipeControl);
};

} // namespace opengl
} // namespace android

// C interface for console
void opengl_pipe_control_stop_pipe(uint64_t);
void opengl_pipe_control_list_pipes(char**); // freed by user

ANDROID_END_HEADER

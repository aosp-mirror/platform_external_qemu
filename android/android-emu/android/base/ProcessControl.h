// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Library to:
// save argv,
// kill and wait a process ID,
// launch process with given cwd and args.

namespace android {
namespace base {

void saveCwdAndArgs(const char* filename);
void killAndWaitProcess(int pid);
void launchWithCwdAndArgs(const char* filename);

} // namespace base
} // namespace android

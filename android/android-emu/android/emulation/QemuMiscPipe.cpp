// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/QemuMiscPipe.h"
#include "android/emulation/AndroidMessagePipe.h"
#include "android/utils/debug.h"
#include "android/globals.h"
#include "android/hw-sensors.h"

#include <assert.h>

#include <memory>
#include <vector>

extern "C" {
 extern void   qemu_system_shutdown_request(void);
}
namespace android {

void qemuMiscPipeDecodeAndExecute(const std::vector<uint8_t>& input,
                               std::vector<uint8_t>* outputp) {
    std::vector<uint8_t> & output = *outputp;
    const char *mesg = (const char*)(&input[0]);
    if (strncmp("bootcomplete", mesg, 12) == 0) {
        output.resize(3);
        output[0]='O';
        output[1]='K';
        output[2]='\0';
        printf("boot completed\n");
        if (android_hw->test_quitAfterBootTimeOut > 0) {
            qemu_system_shutdown_request();
        }

        return;
    }else if (strncmp("get_random=", mesg, 11) == 0) {
        fprintf(stderr, "get_random %s\n", mesg);
        // need to parse the value after =
        int bytes=0;
        sscanf(mesg, "get_random=%d", &bytes);
        if (bytes > 0) {
            output.resize(bytes + 4);
            // fill with random bytes
            srand((unsigned)time(0));
            int i=0;
            int random_integer = rand();
            int *p = (int*)&(output[0]);
            for (i=0; i < output.size() >> 2; ++i) {
                random_integer = rand();
                memcpy(p++, &random_integer, 4);
            }
            output.resize(bytes);
            return;
        }
    }

    //not implemented Kock out
    output.resize(3);
    output[0]='K';
    output[1]='O';
    output[2]='\0';
}

void registerQemuMiscPipeServicePipe() {
    android::AndroidPipe::Service::add(new android::AndroidMessagePipe::Service(
            "QemuMiscPipe", qemuMiscPipeDecodeAndExecute));
}
}  // namespace android

extern "C" void android_init_qemu_misc_pipe(void) {
    android::registerQemuMiscPipeServicePipe();
}

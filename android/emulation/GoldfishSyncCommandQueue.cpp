/* Copyright (C) 2016 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "android/emulation/GoldfishSyncCommandQueue.h"

GoldfishSyncCommandQueue::GoldfishSyncCommandQueue() {
    this->start();
}

void GoldfishSyncCommandQueue::setQueueCommands
    (queue_device_command_t queueCmd,
     device_command_result_t getCmdResult) {
    mQueueCommand = queueCmd;
    mGetCommandResult = getCmdResult;
}

void GoldfishSyncCommandQueue::sendCommandAndGetResult(GoldfishSyncCommand* to_send) {
    DPRINT("call with: "
           "cmd=%d "
           "handle=0x%llx "
           "time=%u ",
           to_send->cmd,
           to_send->handle,
           to_send->time_arg);

#if DEBUG
    uint32_t orig_cmd = to_send->cmd;
#endif

    mInput.send(to_send);

    AutoLock(to_send->lock);

    to_send->done = false;
    while(!to_send->done) {
        to_send->cvDone.wait(&to_send->lock);
    }

    DPRINT("got from sync thread (orig cmd=%u): "
           "cmd=%d "
           "handle=0x%llx "
           "time=%u ",
           orig_cmd,
           to_send->cmd,
           to_send->handle,
           to_send->time_arg);
}

intptr_t GoldfishSyncCommandQueue::main() {
    DPRINT("in sync cmd thread");

    GoldfishSyncCommand* currentCommand;
    uint32_t cmd_out;
    uint64_t handle_out;
    uint32_t time_arg_out;

    uint32_t num_iter = 0;
    while (true) {
        DPRINT("waiting for sync cmd");
        mInput.receive(&currentCommand);
#if DEBUG
        uint32_t origcmd= currentCommand->cmd;
#endif
        DPRINT("run this sync cmd: "
               "cmd=%d "
               "handle=0x%llx "
               "time=%u ",
               currentCommand->cmd,
               currentCommand->handle,
               currentCommand->time_arg);
        num_iter += 1;
        DPRINT("sync cmd thread num iter: %u", num_iter);
        mQueueCommand(currentCommand->cmd,
                      currentCommand->handle,
                      currentCommand->time_arg,
                      &cmd_out, &handle_out, &time_arg_out);
        DPRINT("done with cmd. guest wrote: "
               "origcmd=%u "
               "cmd=%d "
               "handle=0x%llx "
               "time=%u ",
               origcmd,
               cmd_out, handle_out, time_arg_out);
        AutoLock(currentCommand->lock);
        currentCommand->cmd = cmd_out;
        currentCommand->handle = handle_out;
        currentCommand->time_arg = time_arg_out;
        currentCommand->done = true;
        currentCommand->cvDone.signal();
    }

    return 0;
}



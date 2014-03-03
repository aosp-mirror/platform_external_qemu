// Copyright (C) 2011 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/log-rotate.h"
#include "net/net.h"
#include "slirp-android/libslirp.h"
#include "sysemu/sysemu.h"

#include <stdio.h>

#ifdef _WIN32

void qemu_log_rotation_init(void) {
    // Nothing to do on Win32 for now.
}

void qemu_log_rotation_poll(void) {
    // Nothing to do on Win32 for now.
}

#else // !_WIN32

#include <sys/signal.h>

static int rotate_logs_requested = 0;

static void rotate_qemu_logs_handler(int signum) {
  rotate_logs_requested = 1;
}

void qemu_log_rotation_init(void) {
  // Install SIGUSR1 signal handler.
  struct sigaction act;
  sigfillset(&act.sa_mask);
  act.sa_flags = 0;
  act.sa_handler = rotate_qemu_logs_handler;
  if (sigaction(SIGUSR1, &act, NULL) == -1) {
    fprintf(stderr, "Failed to setup SIGUSR1 handler to clear Qemu logs\n");
    exit(-1);
  }
}


// Clears the passed qemu log when the rotate_logs_requested
// is set. We need to clear the logs between the tasks that do not
// require restarting Qemu.
static FILE* rotate_log(FILE* old_log_fd, const char* filename) {
  FILE* new_log_fd = NULL;
  if (old_log_fd) {
    if (fclose(old_log_fd) == -1) {
      fprintf(stderr, "Cannot close old_log fd\n");
      exit(errno);
    }
  }

  if (!filename) {
    fprintf(stderr, "The log filename to be rotated is not provided");
    exit(-1);
  }

  new_log_fd = fopen(filename , "wb+");
  if (new_log_fd == NULL) {
    fprintf(stderr, "Cannot open the log file: %s for write.\n",
            filename);
    exit(1);
  }

  return new_log_fd;
}


void qemu_log_rotation_poll(void) {
    if (!rotate_logs_requested)
        return;

    FILE* new_dns_log_fd = rotate_log(get_slirp_dns_log_fd(),
                                      dns_log_filename);
    FILE* new_drop_log_fd = rotate_log(get_slirp_drop_log_fd(),
                                       drop_log_filename);
    slirp_dns_log_fd(new_dns_log_fd);
    slirp_drop_log_fd(new_drop_log_fd);
    rotate_logs_requested = 0;
}

#endif  // !_WIN32

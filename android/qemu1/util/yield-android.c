#include <errno.h>
#include <stddef.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/select.h>
#endif

// Wait until file descriptor |fd| becomes readable.
void yield_until_fd_readable(int fd) {
    for (;;) {
       fd_set read_fds;
       FD_ZERO(&read_fds);
       FD_SET(fd, &read_fds);
       int ret = select(fd + 1, &read_fds, NULL, NULL, NULL);
       if (ret == 1 || (ret < 0 && errno != -EINTR))
           return;
    }
}


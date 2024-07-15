#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

// From https://msdn.microsoft.com/en-us/library/28d5ce15.aspx
int asprintf(char** buf, const char* format, ...) {
    va_list args;
    int len;

    if (buf == NULL) {
        return -1;
    }

    // retrieve the variable arguments
    va_start(args, format);

    len = _vscprintf(format, args)  // _vscprintf doesn't count
          + 1;                      // terminating '\0'

    if (len <= 0) {
        return len;
    }

    *buf = (char*)malloc(len * sizeof(char));

    vsprintf(*buf, format, args);
    return len;
}
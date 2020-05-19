// this provide some functions written in cpp for windows
// such as wait for more than 64 objects
// work in progress
// main idea:
// using std::async to use the thread pool on windows
// so to avoid creation of threads in g_poll (windows version)

#include <stdio.h>

extern "C" {
// a dummy test function to check the invoking of cpp function from c file
// i.e., oslib-win32.c
int android_base_cpp_print_hello(const char *msg) {
    fprintf(stderr, "called from c file with %s\n", msg);
    return 1;
}
}


#include <stdio.h>
#ifdef _WIN32
#include <rpc.h>
#else
#include <uuid/uuid.h>
#endif

int main(int a, char** b) {
    uuid_t uu;
    uuid_generate(uu);
    printf("hi!\n");
    return 0;
}

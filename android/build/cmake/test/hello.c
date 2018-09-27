#include <stdio.h>
#ifdef _WIN32
#include <rpc.h>
#else
#include <uuid/uuid.h>
#endif

int main(int a, char** b) {
#ifdef _WIN32
    UUID uu;
    UuidCreate(&uu);
#else
    uuid_t uu;
    uuid_generate(uu);
#endif
    printf("hi!\n");
    return 0;
}

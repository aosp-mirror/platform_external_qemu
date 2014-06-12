#include <stdio.h>

using namespace std;


void 
print_help(const char* progname)
{
    if (!progname)
        return;
    printf("Android Wear Connection Agent version 1.0.0\n");
    printf("Usage:\n");
    printf("\t%s\n", progname);
}

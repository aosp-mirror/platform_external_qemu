#ifndef _MSC_VER
#include <unistd.h>
#endif
#include "selinux_internal.h"
#include <stdlib.h>
#include <errno.h>

void freecon(char * con)
{
	free(con);
}

hidden_def(freecon)
